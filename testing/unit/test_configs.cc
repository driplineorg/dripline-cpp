/*
 * test_configs.cc
 *
 *  Created on: May 29, 2026
 *      Author: N.S. Oblath
 */

#include "dripline_config.hh"
#include "monitor_config.hh"
#include "service_config.hh"

#include "catch2/catch_test_macros.hpp"

TEST_CASE( "dripline_config_defaults", "[config]" )
{
    // Pass false to skip reading the optional ~/.dripline_mesh.yaml file
    dripline::dripline_config t_config( false );

    REQUIRE( t_config["broker_port"]().as_uint() == 5672 );
    REQUIRE( t_config["broker"]().as_string() == "localhost" );
    REQUIRE( t_config["requests_exchange"]().as_string() == "requests" );
    REQUIRE( t_config["alerts_exchange"]().as_string() == "alerts" );
    REQUIRE( t_config["heartbeat_routing_key"]().as_string() == "heartbeat" );
    REQUIRE( t_config["max_connection_attempts"]().as_uint() == 10 );
    REQUIRE( t_config.has( "max_payload_size" ) );
}

TEST_CASE( "service_config_defaults", "[config]" )
{
    dripline::service_config t_config( "my_service" );

    // service_config wraps its sub-configs under "dripline_mesh" and adds service-level keys
    REQUIRE( t_config.has( "dripline_mesh" ) );
    REQUIRE( t_config["name"]().as_string() == "my_service" );
    REQUIRE( t_config["loop_timeout_ms"]().as_uint() == 1000 );
    REQUIRE( t_config["message_wait_ms"]().as_uint() == 1000 );
    REQUIRE( t_config["heartbeat_interval_s"]().as_uint() == 60 );
}

TEST_CASE( "service_config_default_name", "[config]" )
{
    // Default name argument is "a_service"
    dripline::service_config t_config;
    REQUIRE( t_config["name"]().as_string() == "a_service" );
}

TEST_CASE( "monitor_config_defaults", "[config]" )
{
    dripline::monitor_config t_config;
    REQUIRE( t_config.has( "dripline_mesh" ) );
}
