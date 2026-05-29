/*
 * test_reply_cache.cc
 *
 *  Created on: May 29, 2026
 *      Author: N.S. Oblath
 */

#include "reply_cache.hh"
#include "return_codes.hh"

#include "param_node.hh"
#include "param_value.hh"

#include "catch2/catch_test_macros.hpp"

TEST_CASE( "reply_cache_set_cache", "[reply_cache]" )
{
    dripline::reply_cache* t_cache = dripline::reply_cache::get_instance();
    REQUIRE( t_cache != nullptr );

    SECTION( "set_cache stores return code, message, and payload" )
    {
        scarab::param_ptr_t t_payload( new scarab::param_value( 42 ) );

        t_cache->set_cache( dripline::dl_amqp_error(), "test message", std::move( t_payload ) );

        REQUIRE( t_cache->ret_code().rc_value() == dripline::dl_amqp_error::s_value );
        REQUIRE( t_cache->return_message() == "test message" );
        REQUIRE( t_cache->payload().as_value().as_uint() == 42u );
    }

    SECTION( "set_cache can be overwritten" )
    {
        t_cache->set_cache( dripline::dl_success(), "first", scarab::param_ptr_t( new scarab::param_value( 1 ) ) );
        t_cache->set_cache( dripline::dl_unhandled_exception(), "second", scarab::param_ptr_t( new scarab::param_value( 2 ) ) );

        REQUIRE( t_cache->ret_code().rc_value() == dripline::dl_unhandled_exception::s_value );
        REQUIRE( t_cache->return_message() == "second" );
        REQUIRE( t_cache->payload().as_value().as_uint() == 2u );
    }
}

TEST_CASE( "reply_cache_free_function", "[reply_cache]" )
{
    // set_reply_cache() is the free-function interface to the singleton
    dripline::set_reply_cache(
        dripline::dl_warning_no_action_taken(),
        "via free function",
        scarab::param_ptr_t( new scarab::param_node() ) );

    dripline::reply_cache* t_cache = dripline::reply_cache::get_instance();
    REQUIRE( t_cache->ret_code().rc_value() == dripline::dl_warning_no_action_taken::s_value );
    REQUIRE( t_cache->return_message() == "via free function" );
    REQUIRE( t_cache->payload().is_node() );
}

TEST_CASE( "reply_cache_assign_throw_reply", "[reply_cache]" )
{
    dripline::reply_cache* t_cache = dripline::reply_cache::get_instance();

    dripline::throw_reply t_src( dripline::dl_resource_error() );
    t_src.set_payload( scarab::param_ptr_t( new scarab::param_value( 99 ) ) );
    t_src << "assigned from throw_reply";

    *t_cache = t_src;   // operator=(const throw_reply&)

    REQUIRE( t_cache->ret_code().rc_value() == dripline::dl_resource_error::s_value );
    REQUIRE( t_cache->return_message() == "assigned from throw_reply" );
    REQUIRE( t_cache->payload().as_value().as_uint() == 99u );
}

TEST_CASE( "reply_cache_singleton_identity", "[reply_cache]" )
{
    // Two calls to get_instance() must return the same pointer
    REQUIRE( dripline::reply_cache::get_instance() == dripline::reply_cache::get_instance() );
}
