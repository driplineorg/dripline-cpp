/*
 * hub.cc
 *
 *  Created on: Jan 7, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "hub.hh"

#include "logger.hh"
#include "parsable.hh"

using scarab::parsable;

using std::string;

namespace dripline
{

    LOGGER( dlog, "hub" );


    hub::hub( const scarab::param_node* a_config, const string& a_queue_name,  const std::string& a_broker_address, unsigned a_port, const std::string& a_auth_file, const bool a_make_connection) :
            service( a_config, a_queue_name, a_broker_address, a_port, a_auth_file, a_make_connection ),
            f_run_handler(),
            f_get_handlers(),
            f_set_handlers(),
            f_cmd_handlers(),
            f_lockout_tag(),
            f_lockout_key( generate_nil_uuid() )
    {
        f_keys.insert( f_queue_name + string( ".#" ) );
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

    reply_info hub::on_request_message( const request_ptr_t a_request )
    {
        reply_package t_reply_pkg( this, a_request );
        // not sure if this is the right way to deal with this thing
        reply_info t_reply_info;

        // the lockout key must be valid
        if( ! a_request->get_lockout_key_valid() )
        {
            LWARN( dlog, "Message had an invalid lockout key" );
            return t_reply_pkg.send_reply( retcode_t::message_error_invalid_key, "Lockout key could not be parsed" );;
        }
        else
        {
            switch( a_request->get_message_op() )
            {
                case op_t::run:
                {
                    //return __do_run_request( a_request, t_reply_pkg );
                    t_reply_info =  __do_run_request( a_request, t_reply_pkg );
                    break;
                }
                case op_t::get:
                {
                    //return __do_get_request( a_request, t_reply_pkg );
                    t_reply_info =  __do_get_request( a_request, t_reply_pkg );
                    break;
                } // end "get" operation
                case op_t::set:
                {
                    //return __do_set_request( a_request, t_reply_pkg );
                    t_reply_info =  __do_set_request( a_request, t_reply_pkg );
                    break;
                } // end "set" operation
                case op_t::cmd:
                {
                    //return  __do_cmd_request( a_request, t_reply_pkg );
                    t_reply_info =  __do_cmd_request( a_request, t_reply_pkg );
                    break;
                }
                default:
                    std::stringstream t_error_stream;
                    t_error_stream << "Unrecognized message operation: <" << a_request->get_message_type() << ">";
                    string t_error_msg( t_error_stream.str() );
                    LWARN( dlog, t_error_msg );
                    //return t_reply_pkg.send_reply( retcode_t::message_error_invalid_method, t_error_msg );
                    t_reply_info = t_reply_pkg.send_reply( retcode_t::message_error_invalid_method, t_error_msg );
                    break;
            } // end switch on message type
            //TODO is this okay, or should each of the methods above implement this in the object they return?
            t_reply_info.f_payload = t_reply_pkg.f_payload;
            return t_reply_info;
        }
        // we shouldn't get here
        return reply_info( false, retcode_t::message_error_invalid_method, "" );
;
    }

    reply_info hub::do_run_request( const request_ptr_t a_request, reply_package& a_reply_pkg )
    {
        return f_run_handler( a_request, a_reply_pkg );
    }

    reply_info hub::do_get_request( const request_ptr_t a_request, reply_package& a_reply_pkg )
    {
        std::string t_query_type = a_request->parsed_rks().front();
        a_request->parsed_rks().pop_front();

        try
        {
            return f_get_handlers.at( t_query_type )( a_request, a_reply_pkg );
        }
        catch( std::out_of_range& e )
        {
            LWARN( dlog, "GET query type <" << t_query_type << "> was not understood (" << e.what() << ")" );
            return a_reply_pkg.send_reply( retcode_t::message_error_bad_payload, "Unrecognized query type or no query type provided: <" + t_query_type + ">" );;
        }
    }

    reply_info hub::do_set_request( const request_ptr_t a_request, reply_package& a_reply_pkg )
    {
        std::string t_set_type = a_request->parsed_rks().front();
        a_request->parsed_rks().pop_front();

        try
        {
            return f_set_handlers.at( t_set_type )( a_request, a_reply_pkg );
        }
        catch( std::out_of_range& e )
        {
            LWARN( dlog, "SET request <" << t_set_type << "> not understood (" << e.what() << ")" );
            return a_reply_pkg.send_reply( retcode_t::message_error_bad_payload, "Unrecognized set request type or no set request type provided: <" + t_set_type + ">" );
        }
    }

    reply_info hub::do_cmd_request( const request_ptr_t a_request, reply_package& a_reply_pkg )
    {
        // get the instruction before checking the lockout key authentication because we need to have the exception for
        // the unlock instruction that allows us to force the unlock.
        std::string t_instruction = a_request->parsed_rks().front();
        a_request->parsed_rks().pop_front();

        try
        {
            return f_cmd_handlers.at( t_instruction )( a_request, a_reply_pkg );
        }
        catch( std::out_of_range& e )
        {
            LWARN( dlog, "CMD instruction <" << t_instruction << "> not understood (" << e.what() << ")" );
            return a_reply_pkg.send_reply( retcode_t::message_error_bad_payload, "Instruction <" + t_instruction + "> not understood" );;
        }
    }

    reply_info hub::__do_run_request( const request_ptr_t a_request, reply_package& a_reply_pkg )
    {
        LDEBUG( dlog, "Run operation request received" );

        if( ! authenticate( a_request->lockout_key() ) )
        {
            std::stringstream t_conv;
            t_conv << a_request->lockout_key();
            string t_message( "Request denied due to lockout (key used: " + t_conv.str() + ")" );
            LINFO( dlog, t_message );
            return a_reply_pkg.send_reply( retcode_t::message_error_access_denied, t_message );;
        }

        return do_run_request( a_request, a_reply_pkg );
    }

    reply_info hub::__do_get_request( request_ptr_t a_request, reply_package& a_reply_pkg )
    {
        LDEBUG( dlog, "Get operation request received" );

        string t_query_type;
        if( ! a_request->parsed_rks().empty() )
        {
            t_query_type = a_request->parsed_rks().front();
        }

        if( t_query_type == "is-locked" )
        {
            a_request->parsed_rks().pop_front();
            return handle_is_locked_request( a_request, a_reply_pkg );
        }

        return do_get_request( a_request, a_reply_pkg );
    }

    reply_info hub::__do_set_request( const request_ptr_t a_request, reply_package& a_reply_pkg )
    {
        LDEBUG( dlog, "Set request received" );

        if( ! authenticate( a_request->lockout_key() ) )
        {
            std::stringstream t_conv;
            t_conv << a_request->lockout_key();
            string t_message( "Request denied due to lockout (key used: " + t_conv.str() + ")" );
            LINFO( dlog, t_message );
            return a_reply_pkg.send_reply( retcode_t::message_error_access_denied, t_message );;
        }

        return do_set_request( a_request, a_reply_pkg );
    }

    reply_info hub::__do_cmd_request( const request_ptr_t a_request, reply_package& a_reply_pkg )
    {
        LDEBUG( dlog, "Cmd request received" );

        string t_instruction;
        if( ! a_request->parsed_rks().empty() )
        {
            t_instruction = a_request->parsed_rks().front();
        }

        //LWARN( mtlog, "uuid string: " << a_request->get_payload().get_value( "key", "") << ", uuid: " << uuid_from_string( a_request->get_payload().get_value( "key", "") ) );
        // this condition includes the exception for the unlock instruction that allows us to force the unlock regardless of the key.
        // disable_key() checks the lockout key if it's not forced, so it's okay that we bypass this call to authenticate() for the unlock instruction.
        if( ! authenticate( a_request->lockout_key() ) && t_instruction != "unlock" && t_instruction != "ping" && t_instruction != "set_condition" )
        {
            std::stringstream t_conv;
            t_conv << a_request->lockout_key();
            string t_message( "Request denied due to lockout (key used: " + t_conv.str() + ")" );
            LINFO( dlog, t_message );
            return a_reply_pkg.send_reply( retcode_t::message_error_access_denied, t_message );;
        }

        if( t_instruction == "lock" )
        {
            a_request->parsed_rks().pop_front();
            return handle_lock_request( a_request, a_reply_pkg );
        }
        else if( t_instruction == "unlock" )
        {
            a_request->parsed_rks().pop_front();
            return handle_unlock_request( a_request, a_reply_pkg );
        }
        else if( t_instruction == "ping" )
        {
            a_request->parsed_rks().pop_front();
            return handle_ping_request( a_request, a_reply_pkg );
        }
        else if( t_instruction == "set_condition" )
        {
            a_request->parsed_rks().pop_front();
            return handle_set_condition_request( a_request, a_reply_pkg );
        }

        return do_cmd_request( a_request, a_reply_pkg );
    }

    reply_info hub::handle_lock_request( const request_ptr_t a_request, reply_package& a_reply_pkg )
    {
        uuid_t t_new_key = enable_lockout( a_request->get_sender_info(), a_request->lockout_key() );
        if( t_new_key.is_nil() )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error, "Unable to lock server" );;
        }

        a_reply_pkg.f_payload.add( "key", new scarab::param_value( string_from_uuid( t_new_key ) ) );
        return a_reply_pkg.send_reply( retcode_t::success, "Server is now locked" );
    }

    reply_info hub::handle_unlock_request( const request_ptr_t a_request, reply_package& a_reply_pkg )
    {
        if( ! is_locked() )
        {
            return a_reply_pkg.send_reply( retcode_t::warning_no_action_taken, "Already unlocked" );
        }

        bool t_force = a_request->get_payload().get_value( "force", false );

        if( disable_lockout( a_request->lockout_key(), t_force ) )
        {
            return a_reply_pkg.send_reply( retcode_t::success, "Server unlocked" );
        }
        return a_reply_pkg.send_reply( retcode_t::device_error, "Failed to unlock server" );;
    }

    reply_info hub::handle_set_condition_request( const request_ptr_t a_request, reply_package& a_reply_pkg )
    {
        return this->__do_handle_set_condition_request( a_request, a_reply_pkg );
    }

    reply_info hub::handle_is_locked_request( const request_ptr_t, reply_package& a_reply_pkg )
    {
        bool t_is_locked = is_locked();
        a_reply_pkg.f_payload.add( "is_locked", scarab::param_value( t_is_locked ) );
        if( t_is_locked ) a_reply_pkg.f_payload.add( "tag", f_lockout_tag );
        return a_reply_pkg.send_reply( retcode_t::success, "Checked lock status" );
    }

    reply_info hub::handle_ping_request( const request_ptr_t a_request, reply_package& a_reply_pkg )
    {
        string t_sender = a_request->sender_package();
        return a_reply_pkg.send_reply( retcode_t::success, "Hello, " + t_sender );
    }


    uuid_t hub::enable_lockout( const scarab::param_node& a_tag, uuid_t a_key )
    {
        if( is_locked() ) return generate_nil_uuid();
        if( a_key.is_nil() ) f_lockout_key = generate_random_uuid();
        else f_lockout_key = a_key;
        f_lockout_tag = a_tag;
        return f_lockout_key;
    }

    bool hub::disable_lockout( const uuid_t& a_key, bool a_force )
    {
        if( ! is_locked() ) return true;
        if( ! a_force && a_key != f_lockout_key ) return false;
        f_lockout_key = generate_nil_uuid();
        f_lockout_tag.clear();
        return true;
    }

    bool hub::authenticate( const uuid_t& a_key ) const
    {
        LDEBUG( dlog, "Authenticating with key <" << a_key << ">" );
        if( is_locked() ) return check_key( a_key );
        return true;
    }


} /* namespace dripline */
