/*
 * test_uuid.cc
 *
 *  Created on: Sept 18, 2024
 *      Author: N.S. Oblath
 */

#include "uuid.hh"

#include "catch.hpp"

TEST_CASE( "uuid", "[message]" )
{
    REQUIRE_NOTHROW( dripline::generate_nil_uuid() );
    REQUIRE_NOTHROW( dripline::generate_random_uuid() );

    REQUIRE_THROWS_AS( dripline::uuid_from_string( "blah" ), std::runtime_error );

    std::string t_test_string_1( "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE" );
    dripline::uuid_t t_test_uuid_1;
    REQUIRE_NOTHROW( t_test_uuid_1 = dripline::uuid_from_string( t_test_string_1 ) );
    REQUIRE_THAT( t_test_string_1, Catch::Equals(dripline::string_from_uuid(t_test_uuid_1), Catch::CaseSensitive::No) );

    std::string t_test_string_2( "{AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE}" );
    dripline::uuid_t t_test_uuid_2;
    REQUIRE_NOTHROW( t_test_uuid_2 = dripline::uuid_from_string( t_test_string_2 ) );
    // compare to t_test_string_1 because the to_string conversion adds dashes and has no braces
    REQUIRE_THAT( t_test_string_1, Catch::Equals(dripline::string_from_uuid(t_test_uuid_2), Catch::CaseSensitive::No) );

    std::string t_test_string_3( "AAAAAAAABBBBCCCCDDDDEEEEEEEEEEEE" );
    dripline::uuid_t t_test_uuid_3;
    REQUIRE_NOTHROW( t_test_uuid_3 = dripline::uuid_from_string( t_test_string_3 ) );
    // compare to t_test_string_1 because the to_string conversion adds dashes and has no braces
    REQUIRE_THAT( t_test_string_1, Catch::Equals(dripline::string_from_uuid(t_test_uuid_3), Catch::CaseSensitive::No) );

    std::string t_test_string_4( "{AAAAAAAABBBBCCCCDDDDEEEEEEEEEEEE}" );
    dripline::uuid_t t_test_uuid_4;
    REQUIRE_NOTHROW( t_test_uuid_4 = dripline::uuid_from_string( t_test_string_4 ) );
    // compare to t_test_string_1 because the to_string conversion adds dashes and has no braces
    REQUIRE_THAT( t_test_string_1, Catch::Equals(dripline::string_from_uuid(t_test_uuid_4), Catch::CaseSensitive::No) );
}




