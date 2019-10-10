/*
 * test_throw_reply.cc
 *
 *  Created on: Oct 8, 2019
 *      Author: N.S. Oblath
 */

#include "dripline_error.hh"

#include "logger.hh"

#include "catch.hpp"

LOGGER( testlog, "test_throw_reply" );

TEST_CASE( "throw_reply", "[error]" )
{
    // simple throw and manual return-code set
    try
    {
        throw dripline::throw_reply( dripline::dl_success() );
    }
    catch( dripline::throw_reply& e )
    {
        REQUIRE( e.ret_code().rc_value() == dripline::dl_success::s_value );
        e.set_return_code( dripline::dl_amqp_error() );
        REQUIRE( e.ret_code().rc_value() == dripline::dl_amqp_error::s_value );
    }

    // reply with message and payload
    try
    {
        dripline::throw_reply t_throw_reply( dripline::dl_success{} );
        scarab::param_ptr_t t_payload( new scarab::param_value( 5 ) );
        t_throw_reply.set_payload( std::move( t_payload ) );
        REQUIRE( t_throw_reply.payload().as_value().as_uint() == 5 );

        // setting the payload with operator=, as you might if you had a reply object you were setting the payload into
        dripline::throw_reply t_payload_from;
        t_payload_from.set_payload( scarab::param_ptr_t(new scarab::param_value( 10 )) );
        t_throw_reply.payload() = std::move(t_payload_from.payload());
        REQUIRE( t_throw_reply.payload().as_value().as_uint() == 10 );

        throw t_throw_reply << "Hello";
    }
    catch( dripline::throw_reply& e )
    {
        REQUIRE( e.ret_code().rc_value() == dripline::dl_success::s_value );
        REQUIRE( std::string(e.what()) == std::string("Return code: success (0) -- Hello") );
        REQUIRE( e.payload().as_value().as_uint() == 5 );

    }
    catch( std::exception& e )
    {
        LERROR( testlog, "Caught standard exception: " << e.what() );
        REQUIRE( false ); // we should never get here
    }
}
