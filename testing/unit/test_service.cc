/*
 * test_service.cc
 *
 *  Created on: Jun 10, 2021
 *      Author: N.S. Oblath
 */

#include "dripline_exceptions.hh"
#include "service.hh"

#include "authentication.hh"
#include "param_node.hh"

#include "catch2/catch_test_macros.hpp"

TEST_CASE( "process_message", "[service]" )
{
    dripline::service t_service( scarab::param_node(), scarab::authentication(), false);

    dripline::request_ptr_t t_request_ptr = dripline::msg_request::create( scarab::param_ptr_t( new scarab::param() ), dripline::op_t::get, "dlcpp_service", "", "" );

    // process_message() now calls submit_message() directly and synchronously.
    // With make_connection=false, no AMQP operations are attempted.
    REQUIRE_NOTHROW( t_service.process_message( t_request_ptr ) );

}




