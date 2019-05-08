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
            f_service( a_service )
    {
    }

    endpoint::~endpoint()
    {
    }

    void endpoint::submit_request_message( const request_ptr_t a_request_ptr)
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

    void endpoint::on_request_message( const request_ptr_t a_request )
    {
        // the lockout key must be valid
        if( ! a_request->get_lockout_key_valid() )
        {
            LWARN( dlog, "Message had an invalid lockout key" );
            send_reply( a_request->reply( dl_message_error_invalid_key(), "Lockout key could not be parsed" ) );
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

        return;
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

    bool endpoint::is_locked() const
    {
        return ! f_service.f_lockout_key.is_nil();
    }

    const scarab::param_node& endpoint::get_lockout_tag() const
    {
        return f_service.f_lockout_tag;
    }

    bool endpoint::check_key( const uuid_t& a_key ) const
    {
        return f_service.f_lockout_key == a_key;
    }

    reply_ptr_t endpoint::handle_ping_request( const request_ptr_t a_request )
    {
        return a_request->reply( dl_success(), "Hello, " + a_request->sender_package() );
    }

} /* namespace dripline */
