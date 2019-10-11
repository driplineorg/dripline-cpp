/*
 * test_amqp.cc
 *
 *  Created on: Sep 25, 2019
 *      Author: N.S. Oblath
 */

#include "amqp.hh"

#include "catch.hpp"

TEST_CASE( "amqp_table", "[amqp]" )
{
    SECTION( "null" )
    {
        scarab::param t_param;
        AmqpClient::TableValue t_conv_table_value = dripline::param_to_table( t_param );
        REQUIRE( t_conv_table_value.GetType() == AmqpClient::TableValue::VT_void );

        scarab::param_ptr_t t_conv2_param = dripline::table_to_param( t_conv_table_value );
        REQUIRE( t_conv2_param->is_null() );
    }

    SECTION( "value" )
    {
        // bool
        scarab::param_value t_value( true );
        AmqpClient::TableValue t_conv_table_value = dripline::param_to_table( t_value );
        REQUIRE( t_conv_table_value.GetType() == AmqpClient::TableValue::VT_bool );
        REQUIRE( t_conv_table_value.GetBool() == t_value.as_bool() );

        scarab::param_ptr_t t_conv2_param = dripline::table_to_param( t_conv_table_value );
        REQUIRE( t_conv2_param->is_value() );
        REQUIRE( t_conv2_param->as_value().is_bool() );
        REQUIRE( t_conv2_param->as_value().as_bool() == t_value.as_bool() );

        // int
        t_value.set< int64_t >( -5 );
        t_conv_table_value = dripline::param_to_table( t_value );
        REQUIRE( t_conv_table_value.GetType() == AmqpClient::TableValue::VT_int64 );
        REQUIRE( t_conv_table_value.GetInt64() == t_value.as_int() );

        t_conv2_param = dripline::table_to_param( t_conv_table_value );
        REQUIRE( t_conv2_param->is_value() );
        REQUIRE( t_conv2_param->as_value().is_int() );
        REQUIRE( t_conv2_param->as_value().as_int() == t_value.as_int() );

        // uint
        t_value.set< uint64_t >( 50U );
        t_conv_table_value = dripline::param_to_table( t_value );
        REQUIRE( t_conv_table_value.GetType() == AmqpClient::TableValue::VT_uint64 );
        REQUIRE( t_conv_table_value.GetUint64() == t_value.as_uint() );
        
        t_conv2_param = dripline::table_to_param( t_conv_table_value );
        REQUIRE( t_conv2_param->is_value() );
        REQUIRE( t_conv2_param->as_value().is_uint() );
        REQUIRE( t_conv2_param->as_value().as_uint() == t_value.as_uint() );

        // double
        t_value.set< double >( 500.0 );
        t_conv_table_value = dripline::param_to_table( t_value );
        REQUIRE( t_conv_table_value.GetType() == AmqpClient::TableValue::VT_double );
        REQUIRE( t_conv_table_value.GetDouble() == t_value.as_double() );

        t_conv2_param = dripline::table_to_param( t_conv_table_value );
        REQUIRE( t_conv2_param->is_value() );
        REQUIRE( t_conv2_param->as_value().is_double() );
        REQUIRE( t_conv2_param->as_value().as_double() == t_value.as_double() );
    }

    SECTION( "node" )
    {
        scarab::param_node t_node;
        t_node.add( "value0", true );
        t_node.add( "value1", -5 );
        t_node.add( "value2", 50U );
        t_node.add( "value3", 500.0 );

        AmqpClient::TableValue t_conv_table_value = dripline::param_to_table( t_node );
        REQUIRE( t_conv_table_value.GetType() == AmqpClient::TableValue::VT_table );

        AmqpClient::Table t_conv_table = t_conv_table_value.GetTable();
        REQUIRE( t_conv_table.size() == t_node.size() );
        REQUIRE( t_conv_table["value0"].GetBool() == t_node["value0"]().as_bool() );
        REQUIRE( t_conv_table["value1"].GetInt64() == t_node["value1"]().as_int() );
        REQUIRE( t_conv_table["value2"].GetUint64() == t_node["value2"]().as_uint() );
        REQUIRE( t_conv_table["value3"].GetDouble() == t_node["value3"]().as_double() );

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

        AmqpClient::TableValue t_conv_table_value = dripline::param_to_table( t_array );
        REQUIRE( t_conv_table_value.GetType() == AmqpClient::TableValue::VT_array );

        AmqpClient::Array t_conv_array = t_conv_table_value.GetArray();
        REQUIRE( t_conv_array.size() == t_array.size() );
        REQUIRE( t_conv_array[0].GetBool() == t_array[0]().as_bool() );
        REQUIRE( t_conv_array[1].GetInt64() == t_array[1]().as_int() );
        REQUIRE( t_conv_array[2].GetUint64() == t_array[2]().as_uint() );
        REQUIRE( t_conv_array[3].GetDouble() == t_array[3]().as_double() );

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

        // conversion to a Table object
        AmqpClient::TableValue t_conv_table_value = dripline::param_to_table( t_node0 );

        // validate structure of the Table object
        REQUIRE( t_conv_table_value.GetType() == AmqpClient::TableValue::VT_table );

        REQUIRE( t_conv_table_value.GetTable()["null1"].GetType() == AmqpClient::TableValue::VT_void );
        REQUIRE( t_conv_table_value.GetTable()["value1"].GetType() == AmqpClient::TableValue::VT_uint64 );
        REQUIRE( t_conv_table_value.GetTable()["array1"].GetType() == AmqpClient::TableValue::VT_array );
        REQUIRE( t_conv_table_value.GetTable()["node1"].GetType() == AmqpClient::TableValue::VT_table );

        REQUIRE( t_conv_table_value.GetTable()["array1"].GetArray()[0].GetType() == AmqpClient::TableValue::VT_uint64 );
        REQUIRE( t_conv_table_value.GetTable()["array1"].GetArray()[1].GetType() == AmqpClient::TableValue::VT_array );
        REQUIRE( t_conv_table_value.GetTable()["array1"].GetArray()[2].GetType() == AmqpClient::TableValue::VT_table );

        REQUIRE( t_conv_table_value.GetTable()["array1"].GetArray()[1].GetArray()[0].GetType() == AmqpClient::TableValue::VT_uint64 );

        REQUIRE( t_conv_table_value.GetTable()["array1"].GetArray()[2].GetTable()["value3"].GetType() == AmqpClient::TableValue::VT_uint64 );
        
        REQUIRE( t_conv_table_value.GetTable()["node1"].GetTable()["value2"].GetType() == AmqpClient::TableValue::VT_uint64 );
        REQUIRE( t_conv_table_value.GetTable()["node1"].GetTable()["array2"].GetType() == AmqpClient::TableValue::VT_array );
        REQUIRE( t_conv_table_value.GetTable()["node1"].GetTable()["node2"].GetType() == AmqpClient::TableValue::VT_table );
        
        REQUIRE( t_conv_table_value.GetTable()["node1"].GetTable()["array2"].GetArray()[0].GetType() == AmqpClient::TableValue::VT_uint64 );

        REQUIRE( t_conv_table_value.GetTable()["node1"].GetTable()["node2"].GetTable()["value3"].GetType() == AmqpClient::TableValue::VT_uint64 );

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
}
