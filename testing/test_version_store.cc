/*
 * test_version_store.cc
 *
 *  Created on: Sep 20, 2019
 *      Author: N.S. Oblath
 */

#include "version_store.hh"

#include "dripline_version.hh"

#include "catch.hpp"

TEST_CASE( "version_store", "[version]" )
{
    dripline::version_store* t_store_ptr = dripline::version_store::get_instance();

    REQUIRE( t_store_ptr->versions().empty() );

    t_store_ptr->add_version< dripline::version >( "dripline-cpp" );

    REQUIRE( t_store_ptr->versions().size() == 1 );
    REQUIRE( t_store_ptr->versions().at("dripline-cpp").package() == "Dripline" );

}

