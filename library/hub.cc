/*
 * hub.cc
 *
 *  Created on: Jan 7, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "hub.hh"

#include "logger.hh"

using std::string;

namespace dripline
{

    LOGGER( dlog, "hub" );


    hub::hub( const scarab::param_node& a_config, const string& a_queue_name,  const std::string& a_broker_address, unsigned a_port, const std::string& a_auth_file, const bool a_make_connection) :
            scarab::cancelable(),
            service( a_config, a_queue_name, a_broker_address, a_port, a_auth_file, a_make_connection ),
            f_run_handler(),
            f_get_handlers(),
            f_set_handlers(),
            f_cmd_handlers()
    {
    }

    hub::~hub()
    {
    }

    void hub::set_run_handler( const handler_func_t& a_func )
    {
        f_run_handler = a_func;
        LDEBUG( dlog, "Set RUN handler" );
        return;
    }

    void hub::register_get_handler( const std::string& a_key, const handler_func_t& a_func )
    {
        f_get_handlers[ a_key ] = a_func;
        LDEBUG( dlog, "Set GET handler for <" << a_key << ">" );
        return;
    }

    void hub::register_set_handler( const std::string& a_key, const handler_func_t& a_func )
    {
        f_set_handlers[ a_key ] = a_func;
        LDEBUG( dlog, "Set SET handler for <" << a_key << ">" );
        return;
    }

    void hub::register_cmd_handler( const std::string& a_key, const handler_func_t& a_func )
    {
        f_cmd_handlers[ a_key ] = a_func;
        LDEBUG( dlog, "Set CMD handler for <" << a_key << ">" );
        return;
    }

    void hub::remove_get_handler( const std::string& a_key )
    {
        if( f_get_handlers.erase( a_key ) == 0 )
        {
            LWARN( dlog, "GET handler <" << a_key << "> was not present; nothing was removed" );
        }
        else
        {
            LDEBUG( dlog, "GET handler <" << a_key << "> was removed" );
        }
        return;
    }

    void hub::remove_set_handler( const std::string& a_key )
    {
        if( f_set_handlers.erase( a_key ) == 0 )
        {
            LWARN( dlog, "SET handler <" << a_key << "> was not present; nothing was removed" );
        }
        else
        {
            LDEBUG( dlog, "SET handler <" << a_key << "> was removed" );
        }
        return;
    }

    void hub::remove_cmd_handler( const std::string& a_key )
    {
        if( f_cmd_handlers.erase( a_key ) == 0 )
        {
            LWARN( dlog, "CMD handler <" << a_key << "> was not present; nothing was removed" );
        }
        else
        {
            LDEBUG( dlog, "CMD handler <" << a_key << "> was removed" );
        }
        return;
    }

    reply_ptr_t hub::do_run_request( const request_ptr_t a_request )
    {
        return f_run_handler( a_request );
    }

    reply_ptr_t hub::do_get_request( const request_ptr_t a_request )
    {
        std::string t_query_type = a_request->parsed_specifier().front();
        a_request->parsed_specifier().pop_front();

        try
        {
            return f_get_handlers.at( t_query_type )( a_request );
        }
        catch( std::out_of_range& e )
        {
            LWARN( dlog, "GET query type <" << t_query_type << "> was not understood (" << e.what() << ")" );
            return a_request->reply( dl_message_error_bad_payload(), "Unrecognized query type or no query type provided: <" + t_query_type + ">" );;
        }
    }

    reply_ptr_t hub::do_set_request( const request_ptr_t a_request )
    {
        std::string t_set_type = a_request->parsed_specifier().front();
        a_request->parsed_specifier().pop_front();

        try
        {
            return f_set_handlers.at( t_set_type )( a_request );
        }
        catch( std::out_of_range& e )
        {
            LWARN( dlog, "SET request <" << t_set_type << "> not understood (" << e.what() << ")" );
            return a_request->reply( dl_message_error_bad_payload(), "Unrecognized set request type or no set request type provided: <" + t_set_type + ">" );
        }
    }

    reply_ptr_t hub::do_cmd_request( const request_ptr_t a_request )
    {
        // get the instruction before checking the lockout key authentication because we need to have the exception for
        // the unlock instruction that allows us to force the unlock.
        std::string t_instruction = a_request->parsed_specifier().front();
        a_request->parsed_specifier().pop_front();

        try
        {
            return f_cmd_handlers.at( t_instruction )( a_request );
        }
        catch( std::out_of_range& e )
        {
            LWARN( dlog, "CMD instruction <" << t_instruction << "> not understood (" << e.what() << ")" );
            return a_request->reply( dl_message_error_bad_payload(), "Instruction <" + t_instruction + "> not understood" );;
        }
    }

} /* namespace dripline */
