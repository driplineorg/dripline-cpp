/*
 * test_agent.cc
 *
 *  Created on: Aug 17, 2018
 *      Author: N.S. Oblath
 */

#include "agent.hh"

#include "catch.hpp"

TEST_CASE( "sub_agent_run", "[agent]" )
{
    std::unique_ptr< dripline::agent > t_agent( new dripline::agent() );
    dripline::agent::sub_agent_run t_sar( t_agent.get() );

    dripline::request_ptr_t t_request = t_sar.sub_agent::create_request();

    REQUIRE( t_request->get_message_type() == dripline::msg_t::request );
    REQUIRE( t_request->get_message_op() == dripline::op_t::run );
}

TEST_CASE( "sub_agent_get", "[agent]" )
{
    std::unique_ptr< dripline::agent > t_agent( new dripline::agent() );
    dripline::agent::sub_agent_get t_sag( t_agent.get() );

    scarab::param_node t_config;
    t_config.add( "value", 5 );

    dripline::request_ptr_t t_request = t_sag.create_request( t_config );

    REQUIRE( t_request->get_message_type() == dripline::msg_t::request );
    REQUIRE( t_request->get_message_op() == dripline::op_t::get );
    REQUIRE( t_request->payload().is_node() );
    REQUIRE( ! t_request->payload().as_node().empty() );
    REQUIRE( t_request->payload().as_node().has( "values" ) );
    REQUIRE( ! t_request->payload().as_node().has( "value" ) );
    REQUIRE( t_request->payload()["values"][0]().as_uint() == 5 );
}

TEST_CASE( "sub_agent_set", "[agent]" )
{
    std::unique_ptr< dripline::agent > t_agent( new dripline::agent() );
    dripline::agent::sub_agent_set t_sas( t_agent.get() );

    SECTION( "No value set" )
    {
        dripline::request_ptr_t t_request = t_sas.sub_agent::create_request();

        REQUIRE( ! t_request );
    }

    SECTION( "Value set" )
    {
        scarab::param_node t_config;
        t_config.add( "value", 5 );

        dripline::request_ptr_t t_request = t_sas.create_request( t_config );

        REQUIRE( t_request->get_message_type() == dripline::msg_t::request );
        REQUIRE( t_request->get_message_op() == dripline::op_t::set );
        REQUIRE( t_request->payload().is_node() );
        REQUIRE( ! t_request->payload().as_node().empty() );
        REQUIRE( t_request->payload().as_node().has( "values" ) );
        REQUIRE( ! t_request->payload().as_node().has( "value" ) );
        REQUIRE( t_request->payload()["values"][0]().as_uint() == 5 );
    }
}

TEST_CASE( "sub_agent_cmd", "[agent]" )
{
    std::unique_ptr< dripline::agent > t_agent( new dripline::agent() );
    dripline::agent::sub_agent_cmd t_sac( t_agent.get() );

    scarab::param_node t_config;
    t_config.add( "value", 5 );

    dripline::request_ptr_t t_request = t_sac.create_request( t_config );

    REQUIRE( t_request->get_message_type() == dripline::msg_t::request );
    REQUIRE( t_request->get_message_op() == dripline::op_t::cmd );
    REQUIRE( t_request->payload().is_node() );
    REQUIRE( ! t_request->payload().as_node().empty() );
    REQUIRE( ! t_request->payload().as_node().has( "values" ) );
    REQUIRE( t_request->payload().as_node().has( "value" ) );
    REQUIRE( t_request->payload()["value"]().as_uint() == 5 );
}

TEST_CASE( "agent", "[agent]" )
{
    std::unique_ptr< dripline::agent > t_agent( new dripline::agent() );
    t_agent->routing_key() = "my_service";
    t_agent->specifier() = "front.back";

    dripline::agent::sub_agent_cmd t_sac( t_agent.get() );

    dripline::request_ptr_t t_request = t_sac.sub_agent::create_request();

    REQUIRE( t_request->routing_key() == "my_service" );
    REQUIRE( t_request->parsed_specifier().front() == "front" );
    REQUIRE( t_request->parsed_specifier().back() == "back" );
}




