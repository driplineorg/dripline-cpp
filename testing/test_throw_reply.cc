/*
 * test_throw_reply.cc
 *
 *  Created on: Oct 8, 2019
 *      Author: N.S. Oblath
 */

#include "dripline_error.hh"

#include "catch.hpp"


TEST_CASE( "throw_reply", "[error]" )
{
    try
    {
        throw dripline::throw_reply( dripline::dl_success() ) ;
    }
    catch( dripline::throw_reply& e )
    {
        REQUIRE( e.ret_code().rc_value() == dripline::dl_success::s_value );
        e.set_return_code( dripline::dl_amqp_error() );
        REQUIRE( e.ret_code().rc_value() == dripline::dl_amqp_error::s_value );
    }
}
