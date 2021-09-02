/*
 * amqp.hh
 *
 *  Created on: Sep 25, 2019
 *      Author: N.S. Oblath
 * 
 * A note on integer handling:
 * 
 * According to a commit message from SimpleAmqpClient v2.5.0, in RabbitMQ uint64 is not supported.  
 * Therefore the largest integer handling we can get is with int64.  Even for unsigned values, this 
 * provides half of 64-bit handling, which is far better than the largest unsigned integer handling, 32 bit.  
 * 
 * Therefore, in translating between the AmqpClient::Table's integers and param's integers, we choose 
 * to always use param's int64 type.  
 * 
 * The full commit message from SimpleAmqpClient follows: 
 * 
 *   lib: add support for table timestamp values & remove broken support for uint64 values.
 * 
 *   uint64_t table values aren't supported by RabbitMQ: they cause
 *   connection errors in older versions of RabbitMQ, and newer brokers
 *   silently convert them to int64_t.
 * 
 *   Timestamps are still represented using uin64_t, so this adds an API that
 *   supports setting Timestamps. The interface is a bit different as
 *   std::time_t is a typedef of another integral type and will conflict with
 *   other overloads in existing overload sets.
 */

#define DRIPLINE_API_EXPORTS

#include "amqp.hh"

namespace dripline
{
    DRIPLINE_API scarab::param_ptr_t table_to_param( const AmqpClient::Table& a_table )
    {
        scarab::param_ptr_t t_param( new scarab::param_node() );
        scarab::param_node& t_node = (*t_param).as_node();
        for( auto i_entry = a_table.begin(); i_entry != a_table.end(); ++i_entry )
        {
            t_node.add( i_entry->first, table_to_param( i_entry->second ) );
        }
        return t_param;
    }

    DRIPLINE_API scarab::param_ptr_t table_to_param( const AmqpClient::Array& a_array )
    {
        scarab::param_ptr_t t_param( new scarab::param_array() );
        scarab::param_array& t_array = (*t_param).as_array();
        for( auto i_entry = a_array.begin(); i_entry != a_array.end(); ++i_entry )
        {
            t_array.push_back( table_to_param( *i_entry ) );
        }
        return t_param;
    }

    DRIPLINE_API scarab::param_ptr_t table_to_param( const AmqpClient::TableValue& a_value )
    {
        using AmqpClient::TableValue;

        switch( a_value.GetType() )
        {
            case TableValue::ValueType::VT_void:
                return scarab::param_ptr_t( new scarab::param() );
                break;
            case TableValue::ValueType::VT_bool:
                return scarab::param_ptr_t( new scarab::param_value( a_value.GetBool() ) );
                break;
            case TableValue::ValueType::VT_int8:
                return scarab::param_ptr_t( new scarab::param_value( a_value.GetInt8() ) );
                break;
            case TableValue::ValueType::VT_int16:
                return scarab::param_ptr_t( new scarab::param_value( a_value.GetInt16() ) );
                break;
            case TableValue::ValueType::VT_int32:
                return scarab::param_ptr_t( new scarab::param_value( a_value.GetInt32() ) );
                break;
            case TableValue::ValueType::VT_int64:
                return scarab::param_ptr_t( new scarab::param_value( a_value.GetInt64() ) );
                break;
            case TableValue::ValueType::VT_float:
                return scarab::param_ptr_t( new scarab::param_value( a_value.GetFloat() ) );
                break;
            case TableValue::ValueType::VT_double:
                return scarab::param_ptr_t( new scarab::param_value( a_value.GetDouble() ) );
                break;
            case TableValue::ValueType::VT_string:
                return scarab::param_ptr_t( new scarab::param_value( a_value.GetString() ) );
                break;
            case TableValue::ValueType::VT_array:
                return table_to_param( a_value.GetArray() );
                break;
            case TableValue::ValueType::VT_table:
                return table_to_param( a_value.GetTable() );
                break;
            case TableValue::ValueType::VT_uint8:
                return scarab::param_ptr_t( new scarab::param_value( a_value.GetUint8() ) );
                break;
            case TableValue::ValueType::VT_uint16:
                return scarab::param_ptr_t( new scarab::param_value( a_value.GetUint16() ) );
                break;
            case TableValue::ValueType::VT_uint32:
                return scarab::param_ptr_t( new scarab::param_value( a_value.GetUint32() ) );
                break;
            case TableValue::ValueType::VT_timestamp:
                return scarab::param_ptr_t( new scarab::param_value( a_value.GetTimestamp() ) );
                break;
            default:
                throw std::domain_error( "Invalid SimpleAMQPClient TableValue type" );
        }
        // should never get here
        return scarab::param_ptr_t();
    }

    DRIPLINE_API AmqpClient::TableValue param_to_table( const scarab::param_node& a_node )
    {
        AmqpClient::Table t_table;
        for( auto i_entry = a_node.begin(); i_entry != a_node.end(); ++i_entry )
        {
            t_table.insert( AmqpClient::TableEntry( i_entry.name(), param_to_table(*i_entry) ) );
        }
        return t_table;
    }

    DRIPLINE_API AmqpClient::TableValue param_to_table( const scarab::param_array& a_array )
    {
        AmqpClient::Array t_array;
        for( auto i_entry = a_array.begin(); i_entry != a_array.end(); ++i_entry )
        {
            t_array.push_back( param_to_table( *i_entry ) );
        }
        return t_array;
    }

    DRIPLINE_API AmqpClient::TableValue param_to_table( const scarab::param_value& a_value )
    {
        if( a_value.is_bool() ) return AmqpClient::TableValue( a_value.as_bool() );
        if( a_value.is_int() ) return AmqpClient::TableValue( a_value.as_int() );
        if( a_value.is_uint() ) return AmqpClient::TableValue( a_value.as_int() );
        if( a_value.is_double() ) return AmqpClient::TableValue( a_value.as_double() );
        if( a_value.is_string() ) return AmqpClient::TableValue( a_value.as_string() );
        throw std::domain_error( "Invalid param value type" );
    }

    DRIPLINE_API AmqpClient::TableValue param_to_table( const scarab::param& a_param )
    {
        if( a_param.is_null() ) return AmqpClient::TableValue();
        if( a_param.is_node() ) return param_to_table( a_param.as_node() );
        if( a_param.is_array() ) return param_to_table( a_param.as_array() );
        if( a_param.is_value() ) return param_to_table( a_param.as_value() );
        throw std::domain_error( "Invalid param type" );
    }


} /* namespace dripline */
