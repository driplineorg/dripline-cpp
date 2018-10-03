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
    REQUIRE( t_rc.rc_value() == 0 );
    REQUIRE( t_rc.rc_value() == dripline::dl_success::value );
}

