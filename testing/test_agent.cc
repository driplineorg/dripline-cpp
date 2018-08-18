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




