/*
 * test_dripline_error.cc
 *
 *  Created on: Oct 10, 2019
 *      Author: N.S. Oblath
 */

#include "return_codes.hh"

#include "dripline_exceptions.hh"

#include "logger.hh"

#include "catch2/catch_test_macros.hpp"

LOGGER( testlog, "test_dripline_error" );

TEST_CASE( "dripline_error", "[exceptions]" )
{
    // check message
    try
    {
        dripline::dripline_error t_error;
        t_error << "testing, testing";
        throw t_error;
    }
    catch(const dripline::dripline_error& e)
    {
        REQUIRE( e.what() == std::string("testing, testing") );
    }
    
    // check CRTP
    // the operator<<() should return a dripline::dripline_error and 
    // not a dripine::base_exception< dripline_error >, 
    // the latter of which we'll check for by catching a std::exception
    try
    {
        throw dripline::dripline_error() << "testing CRTP";
    }
    catch( const dripline::dripline_error& e )
    {
        REQUIRE( true ); // this is where we should end up
    }
    catch( const std::exception& )
    {
        REQUIRE( false ); // we shouldn't get here
    }
}

