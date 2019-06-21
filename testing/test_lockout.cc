/*
 * test_lockout.cc
 *
 *  Created on: Aug 15, 2018
 *      Author: N.S. Oblath
 */

#include "service.hh"

#include "catch.hpp"


TEST_CASE( "lockout", "[endpoint]" ) // even though a service is used here for convenience, lockout functionality lives in endpoint
{
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
}




