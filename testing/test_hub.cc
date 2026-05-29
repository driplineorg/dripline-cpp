/*
 * test_hub.cc
 *
 *  Created on: May 29, 2026
 *      Author: N.S. Oblath
 */

#include "hub.hh"
#include "return_codes.hh"

#include "authentication.hh"
#include "param_node.hh"

#include "catch2/catch_test_macros.hpp"

TEST_CASE( "hub_get_handler", "[hub]" )
{
    dripline::hub t_hub( scarab::param_node(), scarab::authentication(), false );

    SECTION( "registered handler is dispatched" )
    {
        bool t_called = false;
        t_hub.register_get_handler( "my_value", [&t_called]( dripline::request_ptr_t a_req ) {
            t_called = true;
            return a_req->reply( dripline::dl_success(), "ok" );
        } );

        auto t_request = dripline::msg_request::create(
            scarab::param_ptr_t( new scarab::param() ),
            dripline::op_t::get,
            "test.endpoint",
            "my_value",
            "" );

        auto t_reply = t_hub.submit_request_message( t_request );
        REQUIRE( t_called );
        REQUIRE( t_reply->get_return_code() == dripline::dl_success::s_value );
    }

    SECTION( "unrecognized specifier returns bad-payload error" )
    {
        auto t_request = dripline::msg_request::create(
            scarab::param_ptr_t( new scarab::param() ),
            dripline::op_t::get,
            "test.endpoint",
            "not_registered",
            "" );

        auto t_reply = t_hub.submit_request_message( t_request );
        REQUIRE( t_reply->get_return_code() == dripline::dl_service_error_bad_payload::s_value );
    }

    SECTION( "removed handler is no longer dispatched" )
    {
        bool t_called = false;
        t_hub.register_get_handler( "to_remove", [&t_called]( dripline::request_ptr_t a_req ) {
            t_called = true;
            return a_req->reply( dripline::dl_success(), "ok" );
        } );

        t_hub.remove_get_handler( "to_remove" );

        auto t_request = dripline::msg_request::create(
            scarab::param_ptr_t( new scarab::param() ),
            dripline::op_t::get,
            "test.endpoint",
            "to_remove",
            "" );

        auto t_reply = t_hub.submit_request_message( t_request );
        REQUIRE_FALSE( t_called );
        REQUIRE( t_reply->get_return_code() == dripline::dl_service_error_bad_payload::s_value );
    }
}

TEST_CASE( "hub_set_handler", "[hub]" )
{
    dripline::hub t_hub( scarab::param_node(), scarab::authentication(), false );

    SECTION( "registered handler is dispatched" )
    {
        bool t_called = false;
        t_hub.register_set_handler( "my_param", [&t_called]( dripline::request_ptr_t a_req ) {
            t_called = true;
            return a_req->reply( dripline::dl_success(), "set ok" );
        } );

        auto t_request = dripline::msg_request::create(
            scarab::param_ptr_t( new scarab::param() ),
            dripline::op_t::set,
            "test.endpoint",
            "my_param",
            "" );

        auto t_reply = t_hub.submit_request_message( t_request );
        REQUIRE( t_called );
        REQUIRE( t_reply->get_return_code() == dripline::dl_success::s_value );
    }

    SECTION( "unrecognized specifier returns bad-payload error" )
    {
        auto t_request = dripline::msg_request::create(
            scarab::param_ptr_t( new scarab::param() ),
            dripline::op_t::set,
            "test.endpoint",
            "not_registered",
            "" );

        auto t_reply = t_hub.submit_request_message( t_request );
        REQUIRE( t_reply->get_return_code() == dripline::dl_service_error_bad_payload::s_value );
    }

    SECTION( "removed handler is no longer dispatched" )
    {
        bool t_called = false;
        t_hub.register_set_handler( "to_remove", [&t_called]( dripline::request_ptr_t a_req ) {
            t_called = true;
            return a_req->reply( dripline::dl_success(), "ok" );
        } );

        t_hub.remove_set_handler( "to_remove" );

        auto t_request = dripline::msg_request::create(
            scarab::param_ptr_t( new scarab::param() ),
            dripline::op_t::set,
            "test.endpoint",
            "to_remove",
            "" );

        auto t_reply = t_hub.submit_request_message( t_request );
        REQUIRE_FALSE( t_called );
        REQUIRE( t_reply->get_return_code() == dripline::dl_service_error_bad_payload::s_value );
    }
}

TEST_CASE( "hub_cmd_handler", "[hub]" )
{
    dripline::hub t_hub( scarab::param_node(), scarab::authentication(), false );

    SECTION( "registered handler is dispatched" )
    {
        bool t_called = false;
        t_hub.register_cmd_handler( "do_thing", [&t_called]( dripline::request_ptr_t a_req ) {
            t_called = true;
            return a_req->reply( dripline::dl_success(), "cmd ok" );
        } );

        auto t_request = dripline::msg_request::create(
            scarab::param_ptr_t( new scarab::param() ),
            dripline::op_t::cmd,
            "test.endpoint",
            "do_thing",
            "" );

        auto t_reply = t_hub.submit_request_message( t_request );
        REQUIRE( t_called );
        REQUIRE( t_reply->get_return_code() == dripline::dl_success::s_value );
    }

    SECTION( "unrecognized specifier returns bad-payload error" )
    {
        auto t_request = dripline::msg_request::create(
            scarab::param_ptr_t( new scarab::param() ),
            dripline::op_t::cmd,
            "test.endpoint",
            "not_registered",
            "" );

        auto t_reply = t_hub.submit_request_message( t_request );
        REQUIRE( t_reply->get_return_code() == dripline::dl_service_error_bad_payload::s_value );
    }

    SECTION( "removed handler is no longer dispatched" )
    {
        bool t_called = false;
        t_hub.register_cmd_handler( "to_remove", [&t_called]( dripline::request_ptr_t a_req ) {
            t_called = true;
            return a_req->reply( dripline::dl_success(), "ok" );
        } );

        t_hub.remove_cmd_handler( "to_remove" );

        auto t_request = dripline::msg_request::create(
            scarab::param_ptr_t( new scarab::param() ),
            dripline::op_t::cmd,
            "test.endpoint",
            "to_remove",
            "" );

        auto t_reply = t_hub.submit_request_message( t_request );
        REQUIRE_FALSE( t_called );
        REQUIRE( t_reply->get_return_code() == dripline::dl_service_error_bad_payload::s_value );
    }
}
