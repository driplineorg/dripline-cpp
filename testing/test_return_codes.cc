/*
 * test_return_codes.cc
 *
 *  Created on: Aug 26, 2018
 *      Author: N.S. Oblath
 */

#include "return_codes.hh"

#include "dripline_exceptions.hh"

#include "logger.hh"

#include "catch.hpp"

#include <algorithm>
#include <sstream>

LOGGER( testlog, "test_return_codes" );

TEST_CASE( "return_codes", "[exceptions]" )
{
    auto t_factory = scarab::indexed_factory< unsigned, dripline::return_code >::get_instance();

    dripline::dl_success t_rc;
    REQUIRE( t_rc.rc_value() == 0 );
    REQUIRE( t_rc.rc_value() == dripline::dl_success::s_value );
    REQUIRE( t_rc.rc_name() == dripline::dl_success::s_name );
    REQUIRE( t_rc.rc_description() == dripline::dl_success::s_description );

    struct test_unique_error : public ::dripline::return_code
    {
        virtual ~test_unique_error() {}
        virtual unsigned rc_value() const { return 1000; }
        virtual std::string rc_name() const { return "test_unique_error"; }
        virtual std::string rc_description() const {return "Unique Error"; }
    };

    struct test_unique_error_2 : public ::dripline::return_code
    {
        virtual ~test_unique_error_2() {}
        virtual unsigned rc_value() const { return 1001; }
        virtual std::string rc_name() const { return "test_unique_error_2"; }
        virtual std::string rc_description() const { return "Unique Error 2"; }
    };

    struct test_nonunique_error : public ::dripline::return_code
    {
        virtual ~test_nonunique_error() {}
        virtual unsigned rc_value() const { return 100; }
        virtual std::string rc_name() const { return "test_nonunique_error"; }
        virtual std::string rc_description() const { return "Nonunique Error"; }
    };

    REQUIRE_NOTHROW( (scarab::indexed_registrar< unsigned, dripline::return_code, test_unique_error >( 1000 )) );

    // redo the registration, since the registrar doesn't stick around when wrapped in REQUIRE_NOTHROW
    scarab::indexed_registrar< unsigned, dripline::return_code, test_unique_error > t_ue_reg( 1000 );

    // test creating a return-code
    dripline::return_code* t_rc_ue = t_factory->create( 1000 );
    REQUIRE( t_rc_ue != nullptr );
    REQUIRE( t_rc_ue->rc_value() == 1000 );

    REQUIRE_THROWS_AS( (scarab::indexed_registrar< unsigned, dripline::return_code, test_nonunique_error >( 100 )), scarab::error );

    // test operator==
    dripline::dl_success t_rc2;
    REQUIRE( t_rc == t_rc2 );

    // test copy_code
    dripline::copy_code t_copy( t_rc );
    REQUIRE( t_rc == t_copy );

    // test streaming
    std::stringstream t_stream;
    t_stream << t_rc;
    REQUIRE( t_stream.str() == dripline::dl_success::s_description + "(" + std::to_string(dripline::dl_success::s_value) + ")");

    // test creating a custom code with add_return_code
    REQUIRE_NOTHROW( (dripline::add_return_code( 2000, "test_custom_code", "Custom Error" )) );
    REQUIRE( t_factory->has_class( 2000 ) );

    // test creating the custom code
    dripline::return_code* t_rc_cc = t_factory->create( 2000 );
    REQUIRE( t_rc_cc != nullptr );
    REQUIRE( t_rc_cc->rc_value() == 2000 );

    // test adding a duplicate custom code
    REQUIRE_THROWS_AS( (dripline::add_return_code( 2000, "another_custom_code", "Duplicate Error" )), scarab::error );

    // test check_and_add_return_code()
    REQUIRE_FALSE( dripline::check_and_add_return_code( 2000, "another_custom_code_2", "Duplicate Error" ) );
    REQUIRE( dripline::check_and_add_return_code( 2001, "a_space_odyssey", "Fault in the AE-35 unit" ) );
    REQUIRE( t_factory->has_class( 2001 ) );

    // test custom_return_code_registrar
    dripline::custom_return_code_registrar t_crcr( 2010, "cake_not_pie", "No no, piece of cake" );
    REQUIRE( t_factory->has_class( 2010 ) );

    // test get_return_code_values()
    auto t_rc_values = dripline::get_return_code_values();
    REQUIRE( t_rc_values[0] == 0 );
    auto t_find_result = std::find( begin(t_rc_values), end(t_rc_values), 2010 );
    REQUIRE_FALSE( t_find_result == end(t_rc_values) );

    // test get_return_codes_map()
    auto t_rc_map = dripline::get_return_codes_map();
    REQUIRE( t_rc_map[0]->rc_value() == 0 );
    REQUIRE( t_rc_map[2001]->rc_value() == 2001 );
    
}

