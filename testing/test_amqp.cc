/*
 * test_amqp.cc
 *
 *  Created on: Sep 25, 2019
 *      Author: N.S. Oblath
 */

#include "amqp.hh"

#include "bdlt_datetime.h"

#include "catch2/catch_approx.hpp"
#include "catch2/catch_test_macros.hpp"

using namespace BloombergLP;
using Catch::Approx;

TEST_CASE( "amqp_table", "[amqp]" )
{
    SECTION( "null" )
    {
        scarab::param t_param;
        rmqt::FieldValue t_conv_table_value = dripline::param_to_table( t_param );
        REQUIRE( t_conv_table_value.isUnset() );

        scarab::param_ptr_t t_conv2_param = dripline::table_to_param( t_conv_table_value );
        REQUIRE( t_conv2_param->is_null() );
    }

    SECTION( "value" )
    {
        // bool
        scarab::param_value t_value( true );
        rmqt::FieldValue t_conv_table_value = dripline::param_to_table( t_value );
        REQUIRE( t_conv_table_value.is< bool >() );
        REQUIRE( t_conv_table_value.the< bool >() == t_value.as_bool() );

        scarab::param_ptr_t t_conv2_param = dripline::table_to_param( t_conv_table_value );
        REQUIRE( t_conv2_param->is_value() );
        REQUIRE( t_conv2_param->as_value().is_bool() );
        REQUIRE( t_conv2_param->as_value().as_bool() == t_value.as_bool() );

        // int
        t_value.set< int64_t >( -5 );
        t_conv_table_value = dripline::param_to_table( t_value );
        REQUIRE( t_conv_table_value.is< int64_t >() );
        REQUIRE( t_conv_table_value.the< int64_t >() == t_value.as_int() );

        t_conv2_param = dripline::table_to_param( t_conv_table_value );
        REQUIRE( t_conv2_param->is_value() );
        REQUIRE( t_conv2_param->as_value().is_int() );
        REQUIRE( t_conv2_param->as_value().as_int() == t_value.as_int() );

        // uint
        t_value.set< uint32_t >( 50U );
        t_conv_table_value = dripline::param_to_table( t_value );
        REQUIRE( t_conv_table_value.is< uint32_t >() );
        REQUIRE( t_conv_table_value.the< uint32_t >() == t_value.as_uint() );

        t_conv2_param = dripline::table_to_param( t_conv_table_value );
        REQUIRE( t_conv2_param->is_value() );
        REQUIRE( t_conv2_param->as_value().is_uint() );
        REQUIRE( t_conv2_param->as_value().as_uint() == t_value.as_uint() );

        // double
        t_value.set< double >( 500.0 );
        t_conv_table_value = dripline::param_to_table( t_value );
        REQUIRE( t_conv_table_value.is< double >() );
        REQUIRE( t_conv_table_value.the< double >() == t_value.as_double() );

        t_conv2_param = dripline::table_to_param( t_conv_table_value );
        REQUIRE( t_conv2_param->is_value() );
        REQUIRE( t_conv2_param->as_value().is_double() );
        REQUIRE( t_conv2_param->as_value().as_double() == t_value.as_double() );
    }

    SECTION( "string" )
    {
        scarab::param_value t_value( std::string("hello world") );
        rmqt::FieldValue t_conv_table_value = dripline::param_to_table( t_value );
        REQUIRE( t_conv_table_value.is< bsl::string >() );
        REQUIRE( std::string( t_conv_table_value.the< bsl::string >() ) == t_value.as_string() );

        scarab::param_ptr_t t_conv2_param = dripline::table_to_param( t_conv_table_value );
        REQUIRE( t_conv2_param->is_value() );
        REQUIRE( t_conv2_param->as_value().is_string() );
        REQUIRE( t_conv2_param->as_value().as_string() == t_value.as_string() );
    }

    SECTION( "node" )
    {
        scarab::param_node t_node;
        t_node.add( "value0", true );
        t_node.add( "value1", -5 );
        t_node.add( "value2", 50U );
        t_node.add( "value3", 500.0 );

        rmqt::FieldValue t_conv_table_value = dripline::param_to_table( t_node );
        REQUIRE( t_conv_table_value.is< bsl::shared_ptr< rmqt::FieldTable > >() );

        rmqt::FieldTable t_conv_table = *t_conv_table_value.the< bsl::shared_ptr< rmqt::FieldTable > >();
        REQUIRE( t_conv_table.size() == t_node.size() );
        REQUIRE( t_conv_table["value0"].the< bool >() == t_node["value0"]().as_bool() );
        REQUIRE( t_conv_table["value1"].the< int64_t >() == t_node["value1"]().as_int() );
        REQUIRE( t_conv_table["value2"].the< uint32_t >() == t_node["value2"]().as_uint() );
        REQUIRE( t_conv_table["value3"].the< double >() == t_node["value3"]().as_double() );

        scarab::param_ptr_t t_conv2_param = dripline::table_to_param( t_conv_table );
        REQUIRE( t_conv2_param->is_node() );

        scarab::param_node& t_conv2_node = t_conv2_param->as_node();
        REQUIRE( t_conv2_node.size() == t_node.size() );
        REQUIRE( t_conv2_node["value0"]().as_bool() == t_node["value0"]().as_bool() );
        REQUIRE( t_conv2_node["value1"]().as_int() == t_node["value1"]().as_int() );
        REQUIRE( t_conv2_node["value2"]().as_uint() == t_node["value2"]().as_uint() );
        REQUIRE( t_conv2_node["value3"]().as_double() == t_node["value3"]().as_double() );
    }

    SECTION( "array" )
    {
        scarab::param_array t_array;
        t_array.push_back( true );
        t_array.push_back( -5 );
        t_array.push_back( 50U );
        t_array.push_back( 500.0 );

        rmqt::FieldValue t_conv_table_value = dripline::param_to_table( t_array );
        REQUIRE( t_conv_table_value.is< bsl::shared_ptr< rmqt::FieldArray > >() );

        rmqt::FieldArray t_conv_array = *t_conv_table_value.the< bsl::shared_ptr< rmqt::FieldArray > >();
        REQUIRE( t_conv_array.size() == t_array.size() );
        REQUIRE( t_conv_array[0].the< bool >() == t_array[0]().as_bool() );
        REQUIRE( t_conv_array[1].the< int64_t >() == t_array[1]().as_int() );
        REQUIRE( t_conv_array[2].the< uint32_t >() == t_array[2]().as_uint() );
        REQUIRE( t_conv_array[3].the< double >() == t_array[3]().as_double() );

        scarab::param_ptr_t t_conv2_param = dripline::table_to_param( t_conv_array );
        REQUIRE( t_conv2_param->is_array() );

        scarab::param_array& t_conv2_array = t_conv2_param->as_array();
        REQUIRE( t_conv2_array.size() == t_array.size() );
        REQUIRE( t_conv2_array[0]().as_bool() == t_array[0]().as_bool() );
        REQUIRE( t_conv2_array[1]().as_int() == t_array[1]().as_int() );
        REQUIRE( t_conv2_array[2]().as_uint() == t_array[2]().as_uint() );
        REQUIRE( t_conv2_array[3]().as_double() == t_array[3]().as_double() );
    }

    SECTION( "nesting" )
    {
        scarab::param_array t_array2;
        t_array2.push_back( 500U );

        scarab::param_node t_node2;
        t_node2.add( "value3", 500U );

        scarab::param_array t_array1;
        t_array1.push_back( 50U );
        t_array1.push_back( t_array2 );
        t_array1.push_back( t_node2 );

        scarab::param_node t_node1;
        t_node1.add( "value2", 50U );
        t_node1.add( "array2", t_array2 );
        t_node1.add( "node2", t_node2 );

        scarab::param_node t_node0;
        t_node0.add( "null1", scarab::param() );
        t_node0.add( "value1", 5U );
        t_node0.add( "array1", t_array1 );
        t_node0.add( "node1", t_node1 );

        // conversion to a FieldTable object
        rmqt::FieldValue t_conv_table_value = dripline::param_to_table( t_node0 );

        // validate structure of the FieldTable object
        REQUIRE( t_conv_table_value.is< bsl::shared_ptr< rmqt::FieldTable > >() );
        const rmqt::FieldTable& t_ft0 = *t_conv_table_value.the< bsl::shared_ptr< rmqt::FieldTable > >();

        REQUIRE( t_ft0.at("null1").isUnset() );
        REQUIRE( t_ft0.at("value1").is< uint32_t >() );
        REQUIRE( t_ft0.at("array1").is< bsl::shared_ptr< rmqt::FieldArray > >() );
        REQUIRE( t_ft0.at("node1").is< bsl::shared_ptr< rmqt::FieldTable > >() );

        const rmqt::FieldArray& t_fa1 = *t_ft0.at("array1").the< bsl::shared_ptr< rmqt::FieldArray > >();
        REQUIRE( t_fa1[0].is< uint32_t >() );
        REQUIRE( t_fa1[1].is< bsl::shared_ptr< rmqt::FieldArray > >() );
        REQUIRE( t_fa1[2].is< bsl::shared_ptr< rmqt::FieldTable > >() );

        REQUIRE( (*t_fa1[1].the< bsl::shared_ptr< rmqt::FieldArray > >())[0].is< uint32_t >() );
        REQUIRE( t_fa1[2].the< bsl::shared_ptr< rmqt::FieldTable > >()->at("value3").is< uint32_t >() );

        const rmqt::FieldTable& t_ft1 = *t_ft0.at("node1").the< bsl::shared_ptr< rmqt::FieldTable > >();
        REQUIRE( t_ft1.at("value2").is< uint32_t >() );
        REQUIRE( t_ft1.at("array2").is< bsl::shared_ptr< rmqt::FieldArray > >() );
        REQUIRE( t_ft1.at("node2").is< bsl::shared_ptr< rmqt::FieldTable > >() );

        REQUIRE( (*t_ft1.at("array2").the< bsl::shared_ptr< rmqt::FieldArray > >())[0].is< uint32_t >() );
        REQUIRE( t_ft1.at("node2").the< bsl::shared_ptr< rmqt::FieldTable > >()->at("value3").is< uint32_t >() );

        // return conversion to a param object
        scarab::param_ptr_t t_conv2_param = dripline::table_to_param( t_conv_table_value );

        // validate structure and values of the round-trip-converted param object
        REQUIRE( t_conv2_param->is_node() );

        scarab::param_node& t_conv2_node = t_conv2_param->as_node();
        REQUIRE( t_conv2_node.size() == t_node0.size() );

        REQUIRE( t_conv2_node["null1"].is_null() );

        REQUIRE( t_conv2_node["value1"].is_value() );
        REQUIRE( t_conv2_node["value1"]().as_uint() == t_node0["value1"]().as_uint() );

        REQUIRE( t_conv2_node["array1"].is_array() );
        REQUIRE( t_conv2_node["array1"].as_array().size() == t_array1.size() );
        REQUIRE( t_conv2_node["array1"][0].is_value() );
        REQUIRE( t_conv2_node["array1"][0]().as_uint() == t_array1[0]().as_uint() );

        REQUIRE( t_conv2_node["array1"][1].is_array() );
        REQUIRE( t_conv2_node["array1"][1].as_array().size() == t_array2.size() );
        REQUIRE( t_conv2_node["array1"][1][0].is_value() );
        REQUIRE( t_conv2_node["array1"][1][0]().as_uint() == t_array2[0]().as_uint() );

        REQUIRE( t_conv2_node["array1"][2].is_node() );
        REQUIRE( t_conv2_node["array1"][2].as_node().size() == t_node2.size() );
        REQUIRE( t_conv2_node["array1"][2]["value3"].is_value() );
        REQUIRE( t_conv2_node["array1"][2]["value3"]().as_uint() == t_node2["value3"]().as_uint() );

        REQUIRE( t_conv2_node["node1"].is_node() );
        REQUIRE( t_conv2_node["node1"].as_node().size() == t_node1.size() );
        REQUIRE( t_conv2_node["node1"]["value2"].is_value() );
        REQUIRE( t_conv2_node["node1"]["value2"]().as_uint() == t_node1["value2"]().as_uint() );

        REQUIRE( t_conv2_node["node1"]["array2"].is_array() );
        REQUIRE( t_conv2_node["node1"]["array2"].as_array().size() == t_array2.size() );
        REQUIRE( t_conv2_node["node1"]["array2"][0].is_value() );
        REQUIRE( t_conv2_node["node1"]["array2"][0]().as_uint() == t_array2[0]().as_uint() );

        REQUIRE( t_conv2_node["node1"]["node2"].is_node() );
        REQUIRE( t_conv2_node["node1"]["node2"].as_node().size() == t_node2.size() );
        REQUIRE( t_conv2_node["node1"]["node2"]["value3"].is_value() );
        REQUIRE( t_conv2_node["node1"]["node2"]["value3"]().as_uint() == t_node2["value3"]().as_uint() );
    }

    SECTION( "rmqcpp_int_types" )
    {
        // int8_t, int16_t, int32_t → scarab integer
        REQUIRE( dripline::table_to_param( rmqt::FieldValue(int8_t(-1)) )->as_value().is_int() );
        REQUIRE( dripline::table_to_param( rmqt::FieldValue(int8_t(-1)) )->as_value().as_int() == -1 );

        REQUIRE( dripline::table_to_param( rmqt::FieldValue(int16_t(-100)) )->as_value().is_int() );
        REQUIRE( dripline::table_to_param( rmqt::FieldValue(int16_t(-100)) )->as_value().as_int() == -100 );

        REQUIRE( dripline::table_to_param( rmqt::FieldValue(int32_t(-100000)) )->as_value().is_int() );
        REQUIRE( dripline::table_to_param( rmqt::FieldValue(int32_t(-100000)) )->as_value().as_int() == -100000 );

        // uint8_t, uint16_t → scarab uint
        REQUIRE( dripline::table_to_param( rmqt::FieldValue(uint8_t(5)) )->as_value().is_uint() );
        REQUIRE( dripline::table_to_param( rmqt::FieldValue(uint8_t(5)) )->as_value().as_uint() == 5U );

        REQUIRE( dripline::table_to_param( rmqt::FieldValue(uint16_t(5000)) )->as_value().is_uint() );
        REQUIRE( dripline::table_to_param( rmqt::FieldValue(uint16_t(5000)) )->as_value().as_uint() == 5000U );

        // float → scarab double
        REQUIRE( dripline::table_to_param( rmqt::FieldValue(float(1.5f)) )->as_value().is_double() );
        REQUIRE( dripline::table_to_param( rmqt::FieldValue(float(1.5f)) )->as_value().as_double() == Approx(1.5) );
    }

    SECTION( "binary_to_base64" )
    {
        // "Hello" in ASCII bytes encodes to "SGVsbG8=" in base64
        bsl::vector< bsl::uint8_t > t_binary = {0x48, 0x65, 0x6c, 0x6c, 0x6f};
        scarab::param_ptr_t t_result = dripline::table_to_param( rmqt::FieldValue(t_binary) );
        REQUIRE( t_result->is_value() );
        REQUIRE( t_result->as_value().is_string() );
        REQUIRE( t_result->as_value().as_string() == "SGVsbG8=" );

        // empty binary encodes to empty string
        bsl::vector< bsl::uint8_t > t_empty;
        scarab::param_ptr_t t_empty_result = dripline::table_to_param( rmqt::FieldValue(t_empty) );
        REQUIRE( t_empty_result->as_value().as_string() == "" );
    }

    SECTION( "datetime_to_iso8601" )
    {
        bdlt::Datetime t_dt( 2025, 1, 15, 10, 30, 45 );
        scarab::param_ptr_t t_result = dripline::table_to_param( rmqt::FieldValue(t_dt) );
        REQUIRE( t_result->is_value() );
        REQUIRE( t_result->as_value().is_string() );
        // check date and time portion without depending on sub-second format
        REQUIRE( t_result->as_value().as_string().substr( 0, 19 ) == "2025-01-15T10:30:45" );
    }

    SECTION( "unsupported_types" )
    {
        // uint64_t is deprecated in AMQP field tables and has no scarab equivalent
        rmqt::FieldValue t_uint64_val;
        t_uint64_val.assign< uint64_t >( 42ULL );
        REQUIRE_THROWS_AS( dripline::table_to_param( t_uint64_val ), std::domain_error );
    }
}
