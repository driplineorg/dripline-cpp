/*
 * endpoint.cc
 *
 *  Created on: Aug 14, 2018
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "endpoint.hh"

#include "dripline_error.hh"
#include "service.hh"

#include "logger.hh"

LOGGER( dlog, "endpoint" );

namespace dripline
{

    endpoint::endpoint( const std::string& a_name, service& a_service ) :
            f_name( a_name ),
            f_service( a_service ),
            f_lockout_tag(),
            f_lockout_key( generate_nil_uuid() )
    {
    }

    endpoint::~endpoint()
    {
    }

    reply_ptr_t endpoint::submit_request_message( const request_ptr_t a_request_ptr)
    {
        return this->on_request_message( a_request_ptr );;
    }

    void endpoint::submit_alert_message( const alert_ptr_t a_alert_ptr)
    {
        return this->on_alert_message( a_alert_ptr );
    }

    void endpoint::submit_reply_message( const reply_ptr_t a_reply_ptr)
    {
        return this->on_reply_message( a_reply_ptr );
    }

    reply_ptr_t endpoint::on_request_message( const request_ptr_t a_request )
    {
        // the lockout key must be valid
        if( ! a_request->get_lockout_key_valid() )
        {
            LWARN( dlog, "Message had an invalid lockout key" );
            reply_ptr_t t_reply = a_request->reply( dl_message_error_invalid_key(), "Lockout key could not be parsed" );
            send_reply( t_reply );
            return t_reply;
        }

        reply_ptr_t t_reply;
        switch( a_request->get_message_op() )
        {
            case op_t::run:
            {
                t_reply =  __do_run_request( a_request );
                break;
            }
            case op_t::get:
            {
                t_reply =  __do_get_request( a_request );
                break;
            } // end "get" operation
            case op_t::set:
            {
                t_reply =  __do_set_request( a_request );
                break;
            } // end "set" operation
            case op_t::cmd:
            {
                t_reply =  __do_cmd_request( a_request );
                break;
            }
            default:
                std::stringstream t_error_stream;
                t_error_stream << "Unrecognized message operation: <" << a_request->get_message_type() << ">";
                std::string t_error_msg( t_error_stream.str() );
                LWARN( dlog, t_error_msg );
                t_reply = a_request->reply( dl_message_error_invalid_method(), t_error_msg );
                break;
        } // end switch on message type

        if( a_request->reply_to().empty() )
        {
            LWARN( dlog, "Not sending reply (reply-to empty)\n" <<
                         "    Return code: " << t_reply->get_return_code() << '\n' <<
                         "    Return message: " << t_reply->return_msg() << '\n' <<
                         "    Payload:\n" << t_reply->payload() );
        }
        else
        {
            send_reply( t_reply );
        }

        return t_reply;
    }

    void endpoint::send_reply( reply_ptr_t a_reply ) const
    {
        LDEBUG( dlog, "Sending reply message to <" << a_reply->routing_key() << ">:\n" <<
                 "    Return code: " << a_reply->get_return_code() << '\n' <<
                 "    Return message: " << a_reply->return_msg() << '\n' <<
                 "    Payload:\n" << a_reply->payload() );

        if( ! f_service.send( a_reply ) )
        {
            LWARN( dlog, "Something went wrong while sending the reply" );
        }
        return;
    }

    void endpoint::on_reply_message( const reply_ptr_t )
    {
        throw dripline_error() << "Base endpoint does not handle reply messages";
    }

    void endpoint::on_alert_message( const alert_ptr_t )
    {
        throw dripline_error() << "Base endpoint does not handle alert messages";
    }

    reply_ptr_t endpoint::__do_run_request( const request_ptr_t a_request )
    {
        LDEBUG( dlog, "Run operation request received" );

        if( ! f_service.authenticate( a_request->lockout_key() ) )
        {
            std::stringstream t_conv;
            t_conv << a_request->lockout_key();
            std::string t_message( "Request denied due to lockout (key used: " + t_conv.str() + ")" );
            LINFO( dlog, t_message );
            return a_request->reply( dl_message_error_access_denied(), t_message );
        }

        return do_run_request( a_request );
    }

    reply_ptr_t endpoint::__do_get_request( request_ptr_t a_request )
    {
        LDEBUG( dlog, "Get operation request received" );

        std::string t_query_type;
        if( ! a_request->parsed_specifier().empty() )
        {
            t_query_type = a_request->parsed_specifier().front();
        }

        if( t_query_type == "is-locked" )
        {
            a_request->parsed_specifier().pop_front();
            return f_service.handle_is_locked_request( a_request );
        }

        return do_get_request( a_request );
    }

    reply_ptr_t endpoint::__do_set_request( const request_ptr_t a_request )
    {
        LDEBUG( dlog, "Set request received" );

        if( ! f_service.authenticate( a_request->lockout_key() ) )
        {
            std::stringstream t_conv;
            t_conv << a_request->lockout_key();
            std::string t_message( "Request denied due to lockout (key used: " + t_conv.str() + ")" );
            LINFO( dlog, t_message );
            return a_request->reply( dl_message_error_access_denied(), t_message );
        }

        return do_set_request( a_request );
    }

    reply_ptr_t endpoint::__do_cmd_request( const request_ptr_t a_request )
    {
        LDEBUG( dlog, "Cmd request received" );

        std::string t_instruction;
        if( ! a_request->parsed_specifier().empty() )
        {
            t_instruction = a_request->parsed_specifier().front();
        }

        //LWARN( mtlog, "uuid string: " << a_request->get_payload().get_value( "key", "") << ", uuid: " << uuid_from_string( a_request->get_payload().get_value( "key", "") ) );
        // this condition includes the exception for the unlock instruction that allows us to force the unlock regardless of the key.
        // disable_key() checks the lockout key if it's not forced, so it's okay that we bypass this call to authenticate() for the unlock instruction.
        if( ! f_service.authenticate( a_request->lockout_key() ) && t_instruction != "unlock" && t_instruction != "ping" && t_instruction != "set_condition" )
        {
            std::stringstream t_conv;
            t_conv << a_request->lockout_key();
            std::string t_message( "Request denied due to lockout (key used: " + t_conv.str() + ")" );
            LINFO( dlog, t_message );
            return a_request->reply( dl_message_error_access_denied(), t_message );
        }

        if( t_instruction == "lock" )
        {
            a_request->parsed_specifier().pop_front();
            return f_service.handle_lock_request( a_request );
        }
        else if( t_instruction == "unlock" )
        {
            a_request->parsed_specifier().pop_front();
            return f_service.handle_unlock_request( a_request );
        }
        else if( t_instruction == "ping" )
        {
            a_request->parsed_specifier().pop_front();
            return handle_ping_request( a_request );
        }
        else if( t_instruction == "set_condition" )
        {
            a_request->parsed_specifier().pop_front();
            return f_service.handle_set_condition_request( a_request );
        }

        return do_cmd_request( a_request );
    }

    uuid_t endpoint::enable_lockout( const scarab::param_node& a_tag, uuid_t a_key )
    {
        if( is_locked() ) return generate_nil_uuid();
        if( a_key.is_nil() ) f_lockout_key = generate_random_uuid();
        else f_lockout_key = a_key;
        f_lockout_tag = a_tag;
        return f_lockout_key;
    }

    bool endpoint::disable_lockout( const uuid_t& a_key, bool a_force )
    {
        if( ! is_locked() ) return true;
        if( ! a_force && a_key != f_lockout_key ) return false;
        f_lockout_key = generate_nil_uuid();
        f_lockout_tag.clear();
        return true;
    }

    bool endpoint::authenticate( const uuid_t& a_key ) const
    {
        LDEBUG( dlog, "Authenticating with key <" << a_key << ">" );
        if( is_locked() ) return check_key( a_key );
        return true;
    }

    reply_ptr_t endpoint::handle_lock_request( const request_ptr_t a_request )
    {
        uuid_t t_new_key = enable_lockout( a_request->sender_info(), a_request->lockout_key() );
        if( t_new_key.is_nil() )
        {
            return a_request->reply( dl_device_error(), "Unable to lock server" );;
        }

        scarab::param_ptr_t t_payload_ptr( new scarab::param_node() );
        scarab::param_node& t_payload_node = t_payload_ptr->as_node();
        t_payload_node.add( "key", string_from_uuid( t_new_key ) );
        return a_request->reply( dl_success(), "Server is now locked", std::move(t_payload_ptr) );
    }

    reply_ptr_t endpoint::handle_unlock_request( const request_ptr_t a_request )
    {
        if( ! is_locked() )
        {
            return a_request->reply( dl_warning_no_action_taken(), "Already unlocked" );
        }

        bool t_force = a_request->payload().get_value( "force", false );

        if( disable_lockout( a_request->lockout_key(), t_force ) )
        {
            return a_request->reply( dl_success(), "Server unlocked" );
        }
        return a_request->reply( dl_device_error(), "Failed to unlock server" );;
    }

    reply_ptr_t endpoint::handle_set_condition_request( const request_ptr_t a_request )
    {
        return this->__do_handle_set_condition_request( a_request );
    }

    reply_ptr_t endpoint::handle_is_locked_request( const request_ptr_t a_request )
    {
        bool t_is_locked = is_locked();
        scarab::param_ptr_t t_reply_payload( new scarab::param_node() );
        scarab::param_node& t_reply_node = t_reply_payload->as_node();
        t_reply_node.add( "is_locked", t_is_locked );
        if( t_is_locked ) t_reply_node.add( "tag", f_lockout_tag );
        return a_request->reply( dl_success(), "Checked lock status", std::move(t_reply_payload) );
    }

    reply_ptr_t endpoint::handle_ping_request( const request_ptr_t a_request )
    {
        return a_request->reply( dl_success(), "Hello, " + a_request->sender_package() );
    }

} /* namespace dripline */
