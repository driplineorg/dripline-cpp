/*
 * test_endpoint.cc
 *
 *  Created on: Oct 8, 2018
 *      Author: N.S. Oblath
 */

#include "dripline_exceptions.hh"
#include "endpoint.hh"

#include "catch.hpp"

TEST_CASE( "submit_msg", "[endpoint]" )
{
    dripline::endpoint t_endpoint( "test_endpoint" );

    dripline::alert_ptr_t t_alert_ptr = dripline::msg_alert::create(scarab::param_ptr_t(new scarab::param()), "");

    REQUIRE_THROWS_AS( t_endpoint.submit_alert_message( t_alert_ptr ), dripline::dripline_error );

    dripline::request_ptr_t t_request_ptr = dripline::msg_request::create( scarab::param_ptr_t( new scarab::param() ), dripline::op_t::run, "routing.key", "specifier", "" );

    REQUIRE_NOTHROW( t_endpoint.submit_request_message( t_request_ptr ) );

    t_request_ptr->set_message_operation( dripline::op_t::get );

    REQUIRE_NOTHROW( t_endpoint.submit_request_message( t_request_ptr ) );

    t_request_ptr->set_message_operation( dripline::op_t::set );

    REQUIRE_NOTHROW( t_endpoint.submit_request_message( t_request_ptr ) );

    t_request_ptr->set_message_operation( dripline::op_t::cmd );

    REQUIRE_NOTHROW( t_endpoint.submit_request_message( t_request_ptr ) );

}




