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

    dripline::request_ptr_t t_request = t_sar.create_request();

    REQUIRE( t_request->get_message_type() == dripline::msg_t::request );
    REQUIRE( t_request->get_message_op() == dripline::op_t::run );
}

TEST_CASE( "sub_agent_get", "[agent]" )
{
    std::unique_ptr< dripline::agent > t_agent( new dripline::agent() );
    dripline::agent::sub_agent_get t_sag( t_agent.get() );

    t_agent->config().add( "value", 5 );

    dripline::request_ptr_t t_request = t_sag.create_request();

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
        dripline::request_ptr_t t_request = t_sas.create_request();

        REQUIRE( ! t_request );
    }

    SECTION( "Value set" )
    {
        t_agent->config().add( "value", 5 );

        dripline::request_ptr_t t_request = t_sas.create_request();

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

    t_agent->config().add( "value", 5 );

    dripline::request_ptr_t t_request = t_sac.create_request();

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
    dripline::agent t_agent;

    /*
    dripline::service t_service( false );

    REQUIRE( ! t_service.is_locked() );

    SECTION( "locking with a random key" )
    {
        dripline::uuid_t t_key = t_service.enable_lockout( scarab::param_node() );

        REQUIRE( t_service.is_locked() );
        REQUIRE( t_service.check_key( t_key ) );

        dripline::uuid_t t_random_key = dripline::generate_random_uuid();

        REQUIRE( ! t_service.check_key( t_random_key ) );

        t_service.disable_lockout( t_key );

        REQUIRE( ! t_service.is_locked() );
    }

    SECTION( "locking with a specified key" )
    {
        dripline::uuid_t t_key = dripline::generate_random_uuid();
        dripline::uuid_t t_lockedwith_key = t_service.enable_lockout( scarab::param_node(), t_key );

        REQUIRE( t_lockedwith_key == t_key );
        REQUIRE( t_service.is_locked() );
        REQUIRE( t_service.check_key( t_key ) );

        t_service.disable_lockout( t_key );

        REQUIRE( ! t_service.is_locked() );
    }

    SECTION( "unable to lock with nil key" )
    {
        dripline::uuid_t t_key = dripline::generate_nil_uuid();
        dripline::uuid_t t_lockedwith_key = t_service.enable_lockout( scarab::param_node() );

        REQUIRE( t_lockedwith_key != t_key );
        REQUIRE( t_service.is_locked() );
        REQUIRE( t_service.check_key( t_lockedwith_key ) );

        t_service.disable_lockout( t_lockedwith_key );

        REQUIRE( ! t_service.is_locked() );
    }

    */
}




