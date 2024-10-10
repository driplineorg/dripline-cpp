/*
 * test_specifier.cc
 *
 *  Created on: Sep 25, 2018
 *      Author: N.S. Oblath
 */

#include "specifier.hh"

#include "catch2/catch_test_macros.hpp"

TEST_CASE( "specifier", "[message]" )
{
    dripline::specifier t_spec( "path.to.target" );

    REQUIRE( t_spec.unparsed() == "path.to.target" );
    REQUIRE( ! t_spec.empty() );
    REQUIRE( t_spec.size() == 3 );
    REQUIRE( t_spec.front() == "path" );
    REQUIRE( t_spec[1] == "to" );
    REQUIRE( t_spec.back() == "target" );

    t_spec.pop_front();
    REQUIRE( t_spec.size() == 2 );
    REQUIRE( t_spec.front() == "to" );

    t_spec.reparse();
    REQUIRE( t_spec.size() == 3 );
    REQUIRE( t_spec.front() == "path" );
}




