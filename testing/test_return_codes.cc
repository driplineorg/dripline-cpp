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

