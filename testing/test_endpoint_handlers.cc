/*
 * test_endpoint_handlers.cc
 *
 *  Created on: May 29, 2026
 *      Author: N.S. Oblath
 */

#include "dripline_exceptions.hh"
#include "endpoint.hh"
#include "return_codes.hh"
#include "throw_reply.hh"

#include "param_node.hh"

#include "catch2/catch_test_macros.hpp"

// Helper: make a plain request for a given op and specifier
static dripline::request_ptr_t make_request( dripline::op_t a_op, const std::string& a_specifier )
{
    return dripline::msg_request::create(
        scarab::param_ptr_t( new scarab::param() ),
        a_op,
        "test.routing.key",
        a_specifier,
        "" );
}

// -------------------------------------------------------------------------
// Built-in wire-protocol handlers
// -------------------------------------------------------------------------

TEST_CASE( "endpoint_ping", "[endpoint]" )
{
    dripline::endpoint t_endpoint( "test_endpoint" );

    auto t_request = make_request( dripline::op_t::cmd, "ping" );
    auto t_reply = t_endpoint.submit_request_message( t_request );

    REQUIRE( t_reply != nullptr );
    REQUIRE( t_reply->get_return_code() == dripline::dl_success::s_value );
}

TEST_CASE( "endpoint_lock_unlock", "[endpoint]" )
{
    dripline::endpoint t_endpoint( "test_endpoint" );

    SECTION( "lock succeeds on unlocked endpoint" )
    {
        REQUIRE_FALSE( t_endpoint.is_locked() );

        auto t_request = make_request( dripline::op_t::cmd, "lock" );
        auto t_reply = t_endpoint.submit_request_message( t_request );

        REQUIRE( t_reply->get_return_code() == dripline::dl_success::s_value );
        REQUIRE( t_endpoint.is_locked() );
        // reply payload must contain the new lockout key
        REQUIRE( t_reply->payload().is_node() );
        REQUIRE( t_reply->payload().as_node().has( "key" ) );
    }

    SECTION( "lock fails when already locked" )
    {
        // pre-lock using the direct API
        t_endpoint.enable_lockout( scarab::param_node() );
        REQUIRE( t_endpoint.is_locked() );

        auto t_request = make_request( dripline::op_t::cmd, "lock" );
        auto t_reply = t_endpoint.submit_request_message( t_request );

        // handle_lock_request returns dl_resource_error when already locked
        REQUIRE( t_reply->get_return_code() == dripline::dl_resource_error::s_value );
        REQUIRE( t_endpoint.is_locked() );
    }

    SECTION( "force-unlock succeeds regardless of key" )
    {
        t_endpoint.enable_lockout( scarab::param_node() );
        REQUIRE( t_endpoint.is_locked() );

        // payload with force=true; lockout_key in request stays nil (wrong key)
        scarab::param_ptr_t t_payload( new scarab::param_node() );
        t_payload->as_node().add( "force", true );
        auto t_request = dripline::msg_request::create(
            std::move( t_payload ),
            dripline::op_t::cmd,
            "test.routing.key",
            "unlock",
            "" );

        auto t_reply = t_endpoint.submit_request_message( t_request );

        REQUIRE( t_reply->get_return_code() == dripline::dl_success::s_value );
        REQUIRE_FALSE( t_endpoint.is_locked() );
    }

    SECTION( "unlock with correct key succeeds" )
    {
        dripline::uuid_t t_key = t_endpoint.enable_lockout( scarab::param_node() );
        REQUIRE( t_endpoint.is_locked() );

        auto t_request = make_request( dripline::op_t::cmd, "unlock" );
        t_request->lockout_key() = t_key;

        auto t_reply = t_endpoint.submit_request_message( t_request );

        REQUIRE( t_reply->get_return_code() == dripline::dl_success::s_value );
        REQUIRE_FALSE( t_endpoint.is_locked() );
    }

    SECTION( "unlock when already unlocked returns warning" )
    {
        REQUIRE_FALSE( t_endpoint.is_locked() );

        auto t_request = make_request( dripline::op_t::cmd, "unlock" );
        auto t_reply = t_endpoint.submit_request_message( t_request );

        REQUIRE( t_reply->get_return_code() == dripline::dl_warning_no_action_taken::s_value );
    }
}

TEST_CASE( "endpoint_is_locked", "[endpoint]" )
{
    dripline::endpoint t_endpoint( "test_endpoint" );

    SECTION( "is-locked returns false when unlocked" )
    {
        auto t_request = make_request( dripline::op_t::get, "is-locked" );
        auto t_reply = t_endpoint.submit_request_message( t_request );

        REQUIRE( t_reply->get_return_code() == dripline::dl_success::s_value );
        REQUIRE( t_reply->payload().is_node() );
        REQUIRE( t_reply->payload().as_node().has( "is_locked" ) );
        REQUIRE( t_reply->payload().as_node()["is_locked"]().as_bool() == false );
    }

    SECTION( "is-locked returns true when locked" )
    {
        t_endpoint.enable_lockout( scarab::param_node() );

        auto t_request = make_request( dripline::op_t::get, "is-locked" );
        auto t_reply = t_endpoint.submit_request_message( t_request );

        REQUIRE( t_reply->get_return_code() == dripline::dl_success::s_value );
        REQUIRE( t_reply->payload().as_node()["is_locked"]().as_bool() == true );
    }
}

TEST_CASE( "endpoint_set_condition", "[endpoint]" )
{
    dripline::endpoint t_endpoint( "test_endpoint" );

    // default __do_handle_set_condition_request returns dl_success with "No action taken"
    auto t_request = make_request( dripline::op_t::cmd, "set_condition" );
    auto t_reply = t_endpoint.submit_request_message( t_request );

    REQUIRE( t_reply->get_return_code() == dripline::dl_success::s_value );
}

// -------------------------------------------------------------------------
// Lockout authentication enforcement
// -------------------------------------------------------------------------

TEST_CASE( "endpoint_lockout_access_denied", "[endpoint]" )
{
    dripline::endpoint t_endpoint( "test_endpoint" );

    // Lock the endpoint; the stored key is non-nil.  Requests with nil key fail.
    t_endpoint.enable_lockout( scarab::param_node() );
    REQUIRE( t_endpoint.is_locked() );

    SECTION( "set request with wrong key is denied" )
    {
        // default lockout_key on a fresh request is a nil UUID → fails authenticate()
        auto t_request = make_request( dripline::op_t::set, "anything" );
        auto t_reply = t_endpoint.submit_request_message( t_request );

        REQUIRE( t_reply->get_return_code() == dripline::dl_service_error_access_denied::s_value );
    }

    SECTION( "cmd request with wrong key is denied (non-exempt specifier)" )
    {
        auto t_request = make_request( dripline::op_t::cmd, "lock" );
        // lockout_key in request is nil → authenticate() returns false
        auto t_reply = t_endpoint.submit_request_message( t_request );

        REQUIRE( t_reply->get_return_code() == dripline::dl_service_error_access_denied::s_value );
    }

    SECTION( "ping is exempt from lockout" )
    {
        auto t_request = make_request( dripline::op_t::cmd, "ping" );
        auto t_reply = t_endpoint.submit_request_message( t_request );

        REQUIRE( t_reply->get_return_code() == dripline::dl_success::s_value );
    }

    SECTION( "unlock is exempt from lockout" )
    {
        // force-unlock without the correct key should still be allowed through
        scarab::param_ptr_t t_payload( new scarab::param_node() );
        t_payload->as_node().add( "force", true );
        auto t_request = dripline::msg_request::create(
            std::move( t_payload ),
            dripline::op_t::cmd,
            "test.routing.key",
            "unlock",
            "" );
        auto t_reply = t_endpoint.submit_request_message( t_request );

        REQUIRE( t_reply->get_return_code() == dripline::dl_success::s_value );
    }
}

// -------------------------------------------------------------------------
// throw_reply integration: a thrown throw_reply must become the reply
// -------------------------------------------------------------------------

namespace
{
    struct throw_reply_endpoint : public dripline::endpoint
    {
        throw_reply_endpoint() : dripline::endpoint( "throw_reply_ep" ) {}

        dripline::reply_ptr_t do_get_request( const dripline::request_ptr_t a_request ) override
        {
            throw dripline::throw_reply( dripline::dl_amqp_error() ) << "test throw_reply message";
        }
    };
} // anonymous namespace

TEST_CASE( "endpoint_throw_reply_caught", "[endpoint]" )
{
    throw_reply_endpoint t_endpoint;

    auto t_request = make_request( dripline::op_t::get, "anything" );

    // submit_request_message must NOT propagate the throw_reply; it must convert it to a reply
    REQUIRE_NOTHROW( t_endpoint.submit_request_message( t_request ) );

    auto t_reply = t_endpoint.submit_request_message( t_request );
    REQUIRE( t_reply != nullptr );
    REQUIRE( t_reply->get_return_code() == dripline::dl_amqp_error::s_value );
    REQUIRE( t_reply->return_message() == "test throw_reply message" );
}

// -------------------------------------------------------------------------
// endpoint_listener_receiver dispatch
// -------------------------------------------------------------------------

TEST_CASE( "endpoint_listener_receiver_dispatch", "[endpoint]" )
{
    auto t_ep_ptr = std::make_shared< dripline::endpoint >( "test_elr_endpoint" );
    dripline::endpoint_listener_receiver t_elr( t_ep_ptr );

    SECTION( "request message is handled without throwing" )
    {
        dripline::message_ptr_t t_msg = dripline::msg_request::create(
            scarab::param_ptr_t( new scarab::param() ),
            dripline::op_t::get,
            "test.routing.key",
            "",
            "" );
        REQUIRE_NOTHROW( t_elr.submit_message( t_msg ) );
    }

    SECTION( "alert message propagates dripline_error (default handler)" )
    {
        dripline::message_ptr_t t_msg = dripline::msg_alert::create(
            scarab::param_ptr_t( new scarab::param() ),
            "" );
        REQUIRE_THROWS_AS( t_elr.submit_message( t_msg ), dripline::dripline_error );
    }
}
