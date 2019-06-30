/*
 * test_endpoint.cc
 *
 *  Created on: Oct 8, 2018
 *      Author: N.S. Oblath
 */

#include "core.hh"
#include "heartbeater.hh"
#include "service.hh"

#include "macros.hh"

#include "catch.hpp"

#include <future>

TEST_CASE( "heartbeat", "[heartbeat]" )
{
    dripline::core::s_offline = true;

    dripline::heartbeater t_heartbeat;

    // no service given
    REQUIRE_THROWS_AS( t_heartbeat.execute( "test", dripline::generate_random_uuid(), 1, "routing_key" ), dripline::dripline_error );

    dripline::service_ptr_t t_service_ptr = std::make_shared< dripline::service >();
    t_heartbeat.service() = t_service_ptr;

    REQUIRE_THROWS_AS( t_heartbeat.execute( "test", dripline::generate_random_uuid(), 1, "routing_key" ), dripline::message_ptr_t );
}
