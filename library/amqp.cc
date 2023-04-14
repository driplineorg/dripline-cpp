/*
 * amqp.hh
 *
 *  Created on: Sep 25, 2019
 *      Author: N.S. Oblath
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
        if( a_value.is_uint() ) return AmqpClient::TableValue( (uint32_t)a_value.as_uint() );
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
