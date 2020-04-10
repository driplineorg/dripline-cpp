/*
 * test_mock_broker.cc
 *
 *  Created on: Apr 7, 2020
 *      Author: N.S. Oblath
 */


#include "mock_broker.hh"

#include "dripline_exceptions.hh"

#include "catch.hpp"

TEST_CASE( "mock_broker", "[mock_broker]" )
{
    dripline::mock_broker* t_broker = dripline::mock_broker::get_instance();
    // check that we got the broker
    REQUIRE( t_broker != nullptr );



    // add an exchange
    t_broker->declare_exchange( "test_exchange" );
    REQUIRE( t_broker->exchanges().count( "test_exchange" ) == 1 );

    // add a queue
    t_broker->declare_queue( "test_queue" );
    REQUIRE( t_broker->queues().count( "test_queue" ) == 1 );
    // queue should be present
    REQUIRE( t_broker->queues().find( "test_queue" )->second );

    SECTION( "simple test of removing queue and exchange" )
    {
        t_broker->delete_queue( "test_queue" );
        REQUIRE( t_broker->queues().count( "test_queue" ) == 0 );

        t_broker->delete_exchange( "test_exchange" );
        REQUIRE( t_broker->exchanges().count( "test_exchange" ) == 0 );
    }

    t_broker->declare_exchange( "test_exchange" );
    t_broker->declare_queue( "test_queue" );

    // test binding and potential errors
    t_broker->bind( "test_exchange", "test_queue", "test_key" );
    REQUIRE( t_broker->routing_keys().count( "test_exchange" ) == 1 );
    REQUIRE( t_broker->routing_keys().at("test_exchange").count("test_key") == 1 );
    REQUIRE( t_broker->routing_keys().at("test_exchange").at("test_key") == t_broker->queues().at("test_queue") );

    REQUIRE_THROWS_AS( t_broker->bind( "blah", "test_queue", "test_key" ), dripline::dripline_error );
    REQUIRE_THROWS_AS( t_broker->bind( "test_exchange", "blah", "test_key" ), dripline::dripline_error );
    REQUIRE_THROWS_AS( t_broker->bind( "test_exchange", "test_queue", "test_key" ), dripline::dripline_error );

    SECTION( "delete queue while bound" )
    {
        t_broker->delete_queue( "test_queue" );
        REQUIRE( t_broker->queues().count( "test_queue" ) == 0 );
        REQUIRE( t_broker->routing_keys().empty() );
    }

    SECTION( "delete exchange while bound" )
    {
        t_broker->delete_exchange( "test_exchange" );
        REQUIRE( t_broker->exchanges().count( "test_exchange" ) == 0 );
        REQUIRE( t_broker->routing_keys().empty() );
    }

    // TODO: other tests here

    // test unbindings and potential errors
    REQUIRE_THROWS_AS( t_broker->unbind( "blah", "test_queue", "test_key" ), dripline::dripline_error );
    REQUIRE_THROWS_AS( t_broker->unbind( "test_exchange", "blah", "test_key" ), dripline::dripline_error );
    REQUIRE_THROWS_AS( t_broker->unbind( "test_exchange", "test_queue", "blah" ), dripline::dripline_error );
    REQUIRE_NOTHROW( t_broker->unbind( "test_exchange", "test_queue", "test_key" ) );

}
