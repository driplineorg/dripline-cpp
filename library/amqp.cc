/*
 * amqp.hh
 *
 *  Created on: Sep 25, 2019
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "amqp.hh"

#include "bdlde_base64encoder.h"
#include "bdlt_iso8601util.h"

using namespace BloombergLP;

namespace dripline
{
    DRIPLINE_API scarab::param_ptr_t table_to_param( const rmqt::FieldTable& a_table )
    {
        scarab::param_ptr_t t_param( new scarab::param_node() );
        scarab::param_node& t_node = (*t_param).as_node();
        for( auto i_entry = a_table.begin(); i_entry != a_table.end(); ++i_entry )
        {
            t_node.add( std::string(i_entry->first), table_to_param( i_entry->second ) );
        }
        return t_param;
    }

    DRIPLINE_API scarab::param_ptr_t table_to_param( const rmqt::FieldArray& a_array )
    {
        scarab::param_ptr_t t_param( new scarab::param_array() );
        scarab::param_array& t_array = (*t_param).as_array();
        for( auto i_entry = a_array.begin(); i_entry != a_array.end(); ++i_entry )
        {
            t_array.push_back( table_to_param( *i_entry ) );
        }
        return t_param;
    }

    DRIPLINE_API scarab::param_ptr_t table_to_param( const rmqt::FieldValue& a_value )
    {
        if( a_value.isUnset() )
            return scarab::param_ptr_t( new scarab::param() );
        if( a_value.is< bool >() )
            return scarab::param_ptr_t( new scarab::param_value( a_value.the< bool >() ) );
        if( a_value.is< int8_t >() )
            return scarab::param_ptr_t( new scarab::param_value( (int64_t)a_value.the< int8_t >() ) );
        if( a_value.is< int16_t >() )
            return scarab::param_ptr_t( new scarab::param_value( (int64_t)a_value.the< int16_t >() ) );
        if( a_value.is< int32_t >() )
            return scarab::param_ptr_t( new scarab::param_value( (int64_t)a_value.the< int32_t >() ) );
        if( a_value.is< int64_t >() )
            return scarab::param_ptr_t( new scarab::param_value( a_value.the< int64_t >() ) );
        if( a_value.is< uint8_t >() )
            return scarab::param_ptr_t( new scarab::param_value( (uint64_t)a_value.the< uint8_t >() ) );
        if( a_value.is< uint16_t >() )
            return scarab::param_ptr_t( new scarab::param_value( (uint64_t)a_value.the< uint16_t >() ) );
        if( a_value.is< uint32_t >() )
            return scarab::param_ptr_t( new scarab::param_value( (uint64_t)a_value.the< uint32_t >() ) );
        if( a_value.is< float >() )
            return scarab::param_ptr_t( new scarab::param_value( (double)a_value.the< float >() ) );
        if( a_value.is< double >() )
            return scarab::param_ptr_t( new scarab::param_value( a_value.the< double >() ) );
        if( a_value.is< bsl::string >() )
            return scarab::param_ptr_t( new scarab::param_value( std::string( a_value.the< bsl::string >() ) ) );
        if( a_value.is< bsl::shared_ptr< rmqt::FieldArray > >() )
        {
            const auto& t_array_ptr = a_value.the< bsl::shared_ptr< rmqt::FieldArray > >();
            if( ! t_array_ptr ) throw std::domain_error( "Null FieldArray pointer in rmqcpp FieldValue" );
            return table_to_param( *t_array_ptr );
        }
        if( a_value.is< bsl::shared_ptr< rmqt::FieldTable > >() )
        {
            const auto& t_table_ptr = a_value.the< bsl::shared_ptr< rmqt::FieldTable > >();
            if( ! t_table_ptr ) throw std::domain_error( "Null FieldTable pointer in rmqcpp FieldValue" );
            return table_to_param( *t_table_ptr );
        }
        if( a_value.is< bsl::vector< bsl::uint8_t > >() )
        {
            const bsl::vector< bsl::uint8_t >& t_bytes = a_value.the< bsl::vector< bsl::uint8_t > >();
            bdlde::Base64Encoder t_encoder( 0 ); // 0 maxLineLength = no line breaks
            int t_max_len = bdlde::Base64Encoder::encodedLength( (int)t_bytes.size(), 0 );
            bsl::string t_b64( t_max_len, '\0' );
            int t_num_out = 0, t_num_in = 0;
            t_encoder.convert( t_b64.begin(), &t_num_out, &t_num_in,
                               reinterpret_cast< const char* >( t_bytes.data() ),
                               reinterpret_cast< const char* >( t_bytes.data() + t_bytes.size() ) );
            int t_end_out = 0;
            t_encoder.endConvert( t_b64.begin() + t_num_out, &t_end_out );
            t_b64.resize( t_num_out + t_end_out );
            return scarab::param_ptr_t( new scarab::param_value( std::string( t_b64 ) ) );
        }
        if( a_value.is< bdlt::Datetime >() )
        {
            char t_buf[ 64 ];
            int t_len = bdlt::Iso8601Util::generate( t_buf, sizeof( t_buf ), a_value.the< bdlt::Datetime >() );
            if( t_len < 0 ) throw std::domain_error( "Failed to convert bdlt::Datetime to ISO 8601 string" );
            return scarab::param_ptr_t( new scarab::param_value( std::string( t_buf, t_len ) ) );
        }
        // uint64_t (deprecated in AMQP field tables) has no scarab equivalent
        throw std::domain_error( "Unsupported rmqcpp FieldValue type" );
        return scarab::param_ptr_t();
    }

    DRIPLINE_API rmqt::FieldValue param_to_table( const scarab::param_node& a_node )
    {
        auto t_table = bsl::make_shared< rmqt::FieldTable >();
        for( auto i_entry = a_node.begin(); i_entry != a_node.end(); ++i_entry )
        {
            (*t_table)[ bsl::string( i_entry.name() ) ] = param_to_table( *i_entry );
        }
        return rmqt::FieldValue( t_table );
    }

    DRIPLINE_API rmqt::FieldValue param_to_table( const scarab::param_array& a_array )
    {
        auto t_array = bsl::make_shared< rmqt::FieldArray >();
        for( auto i_entry = a_array.begin(); i_entry != a_array.end(); ++i_entry )
        {
            t_array->push_back( param_to_table( *i_entry ) );
        }
        return rmqt::FieldValue( t_array );
    }

    DRIPLINE_API rmqt::FieldValue param_to_table( const scarab::param_value& a_value )
    {
        if( a_value.is_bool() ) return rmqt::FieldValue( a_value.as_bool() );
        if( a_value.is_int() ) return rmqt::FieldValue( (int64_t)a_value.as_int() );
        if( a_value.is_uint() ) return rmqt::FieldValue( (uint32_t)a_value.as_uint() );
        if( a_value.is_double() ) return rmqt::FieldValue( a_value.as_double() );
        if( a_value.is_string() ) return rmqt::FieldValue( bsl::string( a_value.as_string() ) );
        throw std::domain_error( "Invalid param value type" );
    }

    DRIPLINE_API rmqt::FieldValue param_to_table( const scarab::param& a_param )
    {
        if( a_param.is_null() ) return rmqt::FieldValue();
        if( a_param.is_node() ) return param_to_table( a_param.as_node() );
        if( a_param.is_array() ) return param_to_table( a_param.as_array() );
        if( a_param.is_value() ) return param_to_table( a_param.as_value() );
        throw std::domain_error( "Invalid param type" );
    }


} /* namespace dripline */
