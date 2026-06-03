/*
 * test_dripline_constants.cc
 *
 *  Created on: May 29, 2026
 *      Author: N.S. Oblath
 */

#include "dripline_constants.hh"
#include "dripline_exceptions.hh"

#include "catch2/catch_test_macros.hpp"

#include <sstream>

TEST_CASE( "op_t_conversions", "[constants]" )
{
    SECTION( "to_uint" )
    {
        REQUIRE( dripline::to_uint( dripline::op_t::set ) == 0 );
        REQUIRE( dripline::to_uint( dripline::op_t::get ) == 1 );
        REQUIRE( dripline::to_uint( dripline::op_t::cmd ) == 9 );
    }

    SECTION( "to_op_t from uint" )
    {
        REQUIRE( dripline::to_op_t( 0u ) == dripline::op_t::set );
        REQUIRE( dripline::to_op_t( 1u ) == dripline::op_t::get );
        REQUIRE( dripline::to_op_t( 9u ) == dripline::op_t::cmd );
    }

    SECTION( "to_string" )
    {
        REQUIRE( dripline::to_string( dripline::op_t::set ) == "set" );
        REQUIRE( dripline::to_string( dripline::op_t::get ) == "get" );
        REQUIRE( dripline::to_string( dripline::op_t::cmd ) == "cmd" );
        REQUIRE( dripline::to_string( dripline::op_t::unknown ) == "unknown" );
    }

    SECTION( "to_op_t from string - valid" )
    {
        REQUIRE( dripline::to_op_t( std::string("set") ) == dripline::op_t::set );
        REQUIRE( dripline::to_op_t( std::string("get") ) == dripline::op_t::get );
        REQUIRE( dripline::to_op_t( std::string("cmd") ) == dripline::op_t::cmd );
        REQUIRE( dripline::to_op_t( std::string("unknown") ) == dripline::op_t::unknown );
    }

    SECTION( "to_op_t from string - invalid throws" )
    {
        REQUIRE_THROWS_AS( dripline::to_op_t( std::string("invalid") ), dripline::dripline_error );
        REQUIRE_THROWS_AS( dripline::to_op_t( std::string("") ), dripline::dripline_error );
    }

    SECTION( "round-trip uint->op_t->string" )
    {
        REQUIRE( dripline::to_string( dripline::to_op_t( 1u ) ) == "get" );
        REQUIRE( dripline::to_uint( dripline::to_op_t( dripline::to_uint( dripline::op_t::cmd ) ) ) == 9u );
    }

    SECTION( "streaming" )
    {
        std::ostringstream t_os;
        t_os << dripline::op_t::get;
        REQUIRE( t_os.str() == "1" );
    }
}

TEST_CASE( "msg_t_conversions", "[constants]" )
{
    SECTION( "to_uint" )
    {
        REQUIRE( dripline::to_uint( dripline::msg_t::reply ) == 2 );
        REQUIRE( dripline::to_uint( dripline::msg_t::request ) == 3 );
        REQUIRE( dripline::to_uint( dripline::msg_t::alert ) == 4 );
    }

    SECTION( "to_msg_t from uint" )
    {
        REQUIRE( dripline::to_msg_t( 2u ) == dripline::msg_t::reply );
        REQUIRE( dripline::to_msg_t( 3u ) == dripline::msg_t::request );
        REQUIRE( dripline::to_msg_t( 4u ) == dripline::msg_t::alert );
    }

    SECTION( "to_string" )
    {
        REQUIRE( dripline::to_string( dripline::msg_t::reply ) == "reply" );
        REQUIRE( dripline::to_string( dripline::msg_t::request ) == "request" );
        REQUIRE( dripline::to_string( dripline::msg_t::alert ) == "alert" );
        REQUIRE( dripline::to_string( dripline::msg_t::unknown ) == "unknown" );
    }

    SECTION( "to_msg_t from string - valid" )
    {
        REQUIRE( dripline::to_msg_t( std::string("reply") ) == dripline::msg_t::reply );
        REQUIRE( dripline::to_msg_t( std::string("request") ) == dripline::msg_t::request );
        REQUIRE( dripline::to_msg_t( std::string("alert") ) == dripline::msg_t::alert );
        REQUIRE( dripline::to_msg_t( std::string("unknown") ) == dripline::msg_t::unknown );
    }

    SECTION( "to_msg_t from string - invalid throws" )
    {
        REQUIRE_THROWS_AS( dripline::to_msg_t( std::string("invalid") ), dripline::dripline_error );
        REQUIRE_THROWS_AS( dripline::to_msg_t( std::string("") ), dripline::dripline_error );
    }

    SECTION( "round-trip uint->msg_t->string" )
    {
        REQUIRE( dripline::to_string( dripline::to_msg_t( 3u ) ) == "request" );
        REQUIRE( dripline::to_uint( dripline::to_msg_t( dripline::to_uint( dripline::msg_t::alert ) ) ) == 4u );
    }

    SECTION( "streaming" )
    {
        std::ostringstream t_os;
        t_os << dripline::msg_t::reply;
        REQUIRE( t_os.str() == "2" );
    }
}
