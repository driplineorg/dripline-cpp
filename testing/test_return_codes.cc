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

LOGGER( testlog, "test_return_codes" );

TEST_CASE( "return_codes", "[exceptions]" )
{
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

    // redo the registration, since it doesn't stick around when wrapped in REQUIRE_NOTHROW
    scarab::indexed_registrar< unsigned, dripline::return_code, test_unique_error > t_ue_reg( 1000 );

    dripline::return_code* t_rc_ue = scarab::indexed_factory< unsigned, dripline::return_code >::get_instance()->create( 1000 );
    REQUIRE( t_rc_ue != nullptr );
    REQUIRE( t_rc_ue->rc_value() == 1000 );

    REQUIRE_THROWS_AS( (scarab::indexed_registrar< unsigned, dripline::return_code, test_nonunique_error >( 100 )), scarab::error );

}

