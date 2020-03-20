/*
 * return_codes.cc
 *
 *  Created on: Aug 25, 2018
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "return_codes.hh"

#include "logger.hh"

#include <vector>

LOGGER( rclog, "return_codes" );

namespace dripline
{
    copy_code::copy_code( unsigned a_value, const std::string& a_name, const std::string& a_description ) :
            f_value( a_value ),
            f_name( a_name ),
            f_description( a_description )
    {}

    copy_code::copy_code( const return_code& a_code ) :
            f_value( a_code.rc_value() ),
            f_name( a_code.rc_name() ),
            f_description( a_code.rc_description() )
    {}

    bool operator==( const return_code& a_lhs, const return_code& a_rhs )
    {
        return a_lhs.rc_name() == a_rhs.rc_name() &&
               a_lhs.rc_value() == a_rhs.rc_value() &&
               a_lhs.rc_description() == a_rhs.rc_description();
    }

    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, const return_code& a_rc )
    {
        a_os << a_rc.rc_description() << "(" << a_rc.rc_value() << ")";
        return a_os;
    }

    //****************
    // Return code implementations
    //****************

    IMPLEMENT_DL_RET_CODE( success, 0, "Success" );

    IMPLEMENT_DL_RET_CODE( warning_no_action_taken, 1, "No Action Taken" );
    IMPLEMENT_DL_RET_CODE( warning_deprecated_feature, 2, "Deprecated Feature" );
    IMPLEMENT_DL_RET_CODE( warning_dry_run, 3, "Dry Run" );
    IMPLEMENT_DL_RET_CODE( warning_offline, 4, "Offline" );

    IMPLEMENT_DL_RET_CODE( amqp_error, 100, "AMQP Error" );
    IMPLEMENT_DL_RET_CODE( amqp_error_broker_connection, 101, "AMQP Connection Error" );
    IMPLEMENT_DL_RET_CODE( amqp_error_routingkey_notfound, 102, "AMQP Routing Key Error" );

    IMPLEMENT_DL_RET_CODE( device_error, 200, "Device Error" );
    IMPLEMENT_DL_RET_CODE( device_error_connection, 201, "Connection Error" );
    IMPLEMENT_DL_RET_CODE( device_error_no_resp, 202, "No Response Error" );

    IMPLEMENT_DL_RET_CODE( message_error, 300, "Dripline Message Error" );
    IMPLEMENT_DL_RET_CODE( message_error_no_encoding, 301, "No Message Encoding" );
    IMPLEMENT_DL_RET_CODE( message_error_decoding_fail, 302, "Decoding Failed" );
    IMPLEMENT_DL_RET_CODE( message_error_bad_payload, 303, "Bad Payload" );
    IMPLEMENT_DL_RET_CODE( message_error_invalid_value, 304, "Invalid Value" );
    IMPLEMENT_DL_RET_CODE( message_error_timeout, 305, "Timeout Handling Message" );
    IMPLEMENT_DL_RET_CODE( message_error_invalid_method, 306, "Invalid Method" );
    IMPLEMENT_DL_RET_CODE( message_error_access_denied, 307, "Access Denied" );
    IMPLEMENT_DL_RET_CODE( message_error_invalid_key, 308, "Invalid Key" );
    // 309 was formerly "Deprecated Feature"
    IMPLEMENT_DL_RET_CODE( message_error_invalid_specifier, 310, "Invalid Specifier" );

    IMPLEMENT_DL_RET_CODE( client_error, 400, "Client Error" );
    IMPLEMENT_DL_RET_CODE( client_error_invalid_request, 401, "Invalid Request" );
    IMPLEMENT_DL_RET_CODE( client_error_handling_reply, 402, "Error Handling Reply" );
    IMPLEMENT_DL_RET_CODE( client_error_unable_to_send, 403, "Unable to Send" );
    IMPLEMENT_DL_RET_CODE( client_error_timeout, 404, "Client Timeout" );

    IMPLEMENT_DL_RET_CODE( unhandled_exception, 999, "Unhandled Exception" );

    //****************
    // Custom return codes
    //****************

    void add_return_code( unsigned a_value, const std::string& a_name, const std::string& a_description )
    {
        static std::vector< std::unique_ptr<custom_return_code_registrar> > f_rc_registrars;
        f_rc_registrars.emplace_back( new custom_return_code_registrar(a_value, a_name, a_description) );
        return;
    }

    bool check_and_add_return_code( unsigned a_value, const std::string& a_name, const std::string& a_description )
    {
        try
        {
            add_return_code( a_value, a_name, a_description );
            return true;
        }
        catch( const scarab::error& )
        {
            return false;
        }
    }

    std::vector< unsigned > get_return_code_values()
    {
        std::vector< unsigned > return_codes;
        scarab::indexed_factory< unsigned, return_code >* the_factory = scarab::indexed_factory< unsigned, return_code >::get_instance();
        LDEBUG( rclog, "factory is at: " << the_factory );
        for( auto code_entry = the_factory->begin(); code_entry != the_factory->end(); ++code_entry )
        {
            return_codes.push_back( code_entry->first );
        }
        return return_codes;
    }

    std::map< unsigned, std::unique_ptr<return_code> > get_return_codes_map()
    {
        std::map< unsigned, std::unique_ptr<return_code> > the_return_codes;
        scarab::indexed_factory< unsigned, return_code >* the_factory = scarab::indexed_factory< unsigned, return_code >::get_instance();
        for( auto code_entry = the_factory->begin(); code_entry != the_factory->end(); ++code_entry )
        {
            the_return_codes.emplace( std::make_pair( 
                    code_entry->first, 
                    std::unique_ptr<return_code>( code_entry->second->create() )
                ) );
        }
        return the_return_codes;
    }

    custom_return_code_registrar::custom_return_code_registrar( const unsigned& a_value, const std::string& a_name, const std::string& a_description ) :
            scarab::base_registrar< return_code >(),
            f_value( a_value ),
            f_name( a_name ),
            f_description( a_description)
    {
        register_class();
    }

    custom_return_code_registrar::~custom_return_code_registrar()
    {
        scarab::indexed_factory< unsigned, return_code >::get_instance()->remove_class( f_value );
    }

    void custom_return_code_registrar::register_class() const
    {
        scarab::indexed_factory< unsigned, return_code >::get_instance()->register_class( f_value, this );
        return;
    }

    return_code* custom_return_code_registrar::create() const
    {
        return dynamic_cast< return_code* >( new copy_code( f_value, f_name, f_description ) );
    }

} /* namespace dripline */
