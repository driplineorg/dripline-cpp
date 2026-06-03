/*
 * test_version_store.cc
 *
 *  Created on: Sep 20, 2019
 *      Author: N.S. Oblath
 */

#include "version_store.hh"

#include "dripline_version.hh"

#include "catch2/catch_test_macros.hpp"


namespace dripline
{
    class test_version : public scarab::version_semantic
    {
        public:
            test_version() :
                    scarab::version_semantic()
            {
                f_major_ver = 1;
                f_minor_ver = 0;
                f_patch_ver = 0;
                f_version = "v1.0.0";
                f_package = "dripline_test";
                f_commit = "abcdefg";
            }

            ~test_version()
            {}
    };


}


TEST_CASE( "version_store", "[version]" )
{
    // test the things automatically included in the version_store
    // this effectively tests using the adder at static initialization
    dripline::version_store* t_store_ptr = dripline::version_store::get_instance();

    REQUIRE( t_store_ptr->versions().size() == 2 );
    REQUIRE( t_store_ptr->versions().count( "dripline" ) == 1 );
    REQUIRE( t_store_ptr->versions().count( "dripline-cpp" ) == 1 );
    REQUIRE( (t_store_ptr->versions().at("dripline-cpp")->package() == "driplineorg/dripline-cpp" || t_store_ptr->versions().at("dripline-cpp")->package() == "Dripline") );

    // test removing an item
    t_store_ptr->remove_version( "dripline" );

    REQUIRE( t_store_ptr->versions().size() == 1 );
    REQUIRE( t_store_ptr->versions().count( "dripline" ) == 0 );
    REQUIRE( t_store_ptr->versions().count( "dripline-cpp" ) == 1 );

    // test adding via the adder-based add_version function at runtime
    auto t_adder = dripline::add_version< dripline::test_version >( "test-version" );

    REQUIRE( t_store_ptr->versions().size() == 2 );
    REQUIRE( t_store_ptr->versions().count( "test-version" ) == 1 );
    REQUIRE( t_store_ptr->versions().at("test-version")->version_str() == "v1.0.0" );

    // test adding via the polymorphic add_version function
    scarab::version_semantic_ptr_t t_new_ver( new dripline::version_dripline_protocol() );
    dripline::add_version( "dripline", t_new_ver );

    REQUIRE( t_store_ptr->versions().size() == 3 );
    REQUIRE( t_store_ptr->versions().count( "dripline" ) == 1 );
}

