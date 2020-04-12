/*
 * test_mock_broker.cc
 *
 *  Created on: Apr 7, 2020
 *      Author: N.S. Oblath
 */


#include "mock_broker.hh"

#include "dripline_exceptions.hh"
#include "message.hh"

#include "catch.hpp"

TEST_CASE( "broker_binding", "[mock_broker]" )
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
    REQUIRE_NOTHROW( t_broker->bind( "test_exchange", "test_queue", "test_key" ) );
    REQUIRE( t_broker->routing_keys().count( "test_exchange" ) == 1 );
    REQUIRE( t_broker->routing_keys().at("test_exchange").count("test_key") == 1 );
    REQUIRE( t_broker->routing_keys().at("test_exchange").at("test_key") == t_broker->queues().at("test_queue") );

    // test binding check
    REQUIRE( t_broker->is_bound( "test_exchange", "test_queue", "test_key" ) );
    REQUIRE( ! t_broker->is_bound( "blah", "test_queue", "test_key" ) );
    REQUIRE( ! t_broker->is_bound( "test_exchange", "blah", "test_key" ) );
    REQUIRE( ! t_broker->is_bound( "test_exchange", "test_queue", "blah" ) );

    // test invalid bindings
    REQUIRE_THROWS_AS( t_broker->bind( "blah", "test_queue", "test_key" ), dripline::dripline_error );
    REQUIRE_THROWS_AS( t_broker->bind( "test_exchange", "blah", "test_key" ), dripline::dripline_error );
    REQUIRE_THROWS_AS( t_broker->bind( "test_exchange", "test_queue", "test_key" ), dripline::dripline_error );

    // test invalid and valid unbindings
    REQUIRE_THROWS_AS( t_broker->unbind( "blah", "test_queue", "test_key" ), dripline::dripline_error );
    REQUIRE_THROWS_AS( t_broker->unbind( "test_exchange", "blah", "test_key" ), dripline::dripline_error );
    REQUIRE_THROWS_AS( t_broker->unbind( "test_exchange", "test_queue", "blah" ), dripline::dripline_error );
    REQUIRE_NOTHROW( t_broker->unbind( "test_exchange", "test_queue", "test_key" ) );
    REQUIRE( ! t_broker->is_bound( "test_exchange", "test_queue", "test_key" ) );

    // delete queue while bound
    REQUIRE_NOTHROW( t_broker->bind( "test_exchange", "test_queue", "test_key" ) );
    REQUIRE( t_broker->is_bound( "test_exchange", "test_queue", "test_key" ) );
    REQUIRE_NOTHROW( t_broker->delete_queue( "test_queue" ) );
    REQUIRE( t_broker->queues().count( "test_queue" ) == 0 );
    REQUIRE( t_broker->routing_keys().empty() );
    REQUIRE( ! t_broker->is_bound( "test_exchange", "test_queue", "test_key" ) );

    // delete exchange while bound
    REQUIRE_NOTHROW( t_broker->declare_queue( "test_queue" ) );
    REQUIRE_NOTHROW( t_broker->bind( "test_exchange", "test_queue", "test_key" ) );
    REQUIRE( t_broker->is_bound( "test_exchange", "test_queue", "test_key" ) );
    REQUIRE_NOTHROW( t_broker->delete_exchange( "test_exchange" ) );
    REQUIRE( t_broker->exchanges().count( "test_exchange" ) == 0 );
    REQUIRE( t_broker->routing_keys().empty() );
    REQUIRE( ! t_broker->is_bound( "test_exchange", "test_queue", "test_key" ) );

}

TEST_CASE( "broker_use", "[mock_broker]" )
{
    dripline::mock_broker* t_broker = dripline::mock_broker::get_instance();

    dripline::message_ptr_t t_message = dripline::msg_request::create( scarab::param_ptr_t(), dripline::op_t::cmd, "test_key" );

    // test missing exchange and key
    REQUIRE_THROWS_AS( t_broker->send( t_message, "test_exchange" ), dripline::dripline_error );
    t_broker->declare_exchange( "test_exchange" );
    REQUIRE_THROWS_AS( t_broker->send( t_message, "test_exchange" ), dripline::dripline_error );

    t_broker->declare_queue( "test_queue" );
    REQUIRE( t_broker->queues()["test_queue"]->empty() );

    t_broker->bind( "test_exchange", "test_queue", "test_key" );

    // sending

    // non-existent exchange
    REQUIRE_THROWS_AS( t_broker->send( t_message, "blah" ), dripline::dripline_error );

    REQUIRE_NOTHROW( t_broker->send( t_message, "test_exchange" ) );
    REQUIRE( t_broker->queues()["test_queue"]->size() == 1 );

    // consuming

    // non-existent queue
    REQUIRE_THROWS_AS( t_broker->consume( "blah" ), dripline::dripline_error );

    dripline::message_ptr_t t_received_message;
    REQUIRE_NOTHROW( t_received_message = t_broker->consume( "test_queue" ) );

    // no message present
    REQUIRE_THROWS_AS( t_received_message = t_broker->consume( "test_queue", 10 ), dripline::dripline_error );
}
