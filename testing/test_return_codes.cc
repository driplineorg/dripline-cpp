/*
 * test_return_codes.cc
 *
 *  Created on: Aug 26, 2018
 *      Author: N.S. Oblath
 */

#include "return_codes.hh"

#include "catch.hpp"

TEST_CASE( "return_codes", "[core]" )
{
    dripline::dl_success t_rc;
    REQUIRE( t_rc.retcode() == 0 );
}

TEST_CASE( "reply_package_2", "[core]" )
{
    dripline::reply_package_2 t_rp;
    t_rp.set_retcode< dripline::dl_success >() << "Test message";
    t_rp.payload().as_node().add( "test_value", 5 );

    REQUIRE( t_rp.retcode() == 0 );
    REQUIRE( t_rp.message() == "Test message" );
    REQUIRE( t_rp.payload().is_node() );
    REQUIRE( t_rp.payload()["test_value"]().as_int() == 5 );
}

