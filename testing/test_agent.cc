/*
 * test_agent.cc
 *
 *  Created on: Aug 17, 2018
 *      Author: N.S. Oblath
 */

#include "agent.hh"
#include "agent_config.hh"

#include "application.hh"

//#include "logger.hh"

#include "catch.hpp"

//LOGGER( talog, "test_agent" );

TEST_CASE( "agent_configuration", "[agent]" )
{
    using Catch::Matchers::Equals;

    // This setup should match what's done in dl_agent.cc

    scarab::main_app the_main;
    dripline::agent the_agent;

    the_main.default_config() = dripline::agent_config();
    // Dripline authentication specification
    dripline::add_dripline_auth_spec( the_main );

    // end setup

    REQUIRE( the_main.default_config()["timeout"]().as_int() == 10 );
    REQUIRE( the_main.default_config()["dripline_mesh"].is_node() );

    REQUIRE( the_main.default_config().has("auth-groups") );
    REQUIRE_THAT( the_main.default_config()["auth-groups"]["dripline"]["username"]["default"]().as_string(), Equals("guest") );
    REQUIRE_THAT( the_main.default_config()["auth-groups"]["dripline"]["password"]["default"]().as_string(), Equals("guest") );

    // pre_callback() runs the configuration stages and authentication step
    the_main.pre_callback();

    REQUIRE( the_main.primary_config()["timeout"]().as_int() == 10 );
    REQUIRE( the_main.primary_config()["dripline_mesh"].is_node() );
    REQUIRE_FALSE( the_main.primary_config().has("auth-groups") ); // auth-groups should have been stripped out after authentication was handled by the_main
}

TEST_CASE( "sub_agent_get", "[agent]" )
{
    std::unique_ptr< dripline::agent > t_agent( new dripline::agent() );
    dripline::agent::sub_agent_get t_sag( t_agent.get() );

    scarab::param_node t_config;
    dripline::request_ptr_t t_request = t_sag.create_request( t_config );

    REQUIRE( t_request );
    REQUIRE( t_request->get_message_type() == dripline::msg_t::request );
    REQUIRE( t_request->get_message_operation() == dripline::op_t::get );
}

TEST_CASE( "sub_agent_set", "[agent]" )
{
    std::unique_ptr< dripline::agent > t_agent( new dripline::agent() );
    dripline::agent::sub_agent_set t_sas( t_agent.get() );

    SECTION( "No value set" )
    {
        scarab::param_node t_config;
        dripline::request_ptr_t t_request = t_sas.create_request( t_config );

        REQUIRE( ! t_request );
    }

    SECTION( "Value set" )
    {
        scarab::param_array t_values;
        t_values.push_back( 5 );
        scarab::param_node t_config;
        t_config.add( "values", t_values );

        dripline::request_ptr_t t_request = t_sas.create_request( t_config );

        REQUIRE( t_request );
        REQUIRE( t_request->get_message_type() == dripline::msg_t::request );
        REQUIRE( t_request->get_message_operation() == dripline::op_t::set );
        REQUIRE( t_request->payload().is_node() );
        REQUIRE( ! t_request->payload().as_node().empty() );
        REQUIRE( t_request->payload().as_node().has( "values" ) );
        REQUIRE( t_request->payload()["values"][0]().as_uint() == 5 );
    }
}

TEST_CASE( "sub_agent_cmd", "[agent]" )
{
    std::unique_ptr< dripline::agent > t_agent( new dripline::agent() );
    dripline::agent::sub_agent_cmd t_sac( t_agent.get() );

    scarab::param_node t_config;
    dripline::request_ptr_t t_request = t_sac.create_request( t_config );

    REQUIRE( t_request );
    REQUIRE( t_request->get_message_type() == dripline::msg_t::request );
    REQUIRE( t_request->get_message_operation() == dripline::op_t::cmd );
}

TEST_CASE( "agent", "[agent]" )
{
    std::unique_ptr< dripline::agent > t_agent( new dripline::agent() );
    t_agent->routing_key() = "my_service";
    t_agent->specifier() = "front.back";

    dripline::agent::sub_agent_cmd t_sac( t_agent.get() );

    scarab::param_node t_config;
    dripline::request_ptr_t t_request = t_sac.create_request( t_config );

    REQUIRE( t_request );
    REQUIRE( t_request->routing_key() == "my_service" );
    REQUIRE( t_request->parsed_specifier().front() == "front" );
    REQUIRE( t_request->parsed_specifier().back() == "back" );
}




