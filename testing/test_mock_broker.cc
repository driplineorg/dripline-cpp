/*
 * test_mock_broker.cc
 *
 *  Created on: Apr 7, 2020
 *      Author: N.S. Oblath
 */


#include "mock_broker.hh"

#include "catch.hpp"

TEST_CASE( "mock_broker", "[mock_broker]" )
{
    dripline::mock_broker* t_broker = dripline::mock_broker::get_instance();

    REQUIRE( t_broker != nullptr );

}
