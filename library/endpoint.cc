/*
 * endpoint.cc
 *
 *  Created on: Aug 14, 2018
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "endpoint.hh"

#include "dripline_exceptions.hh"
#include "service.hh"
#include "throw_reply.hh"

#include "logger.hh"

#ifdef DL_PYTHON
#include "reply_cache.hh"

#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"
#endif

LOGGER( dlog, "endpoint" );

namespace dripline
{

    endpoint::endpoint( const std::string& a_name ) :
            f_name( a_name ),
            f_service(),
            f_lockout_tag(),
            f_lockout_key( generate_nil_uuid() )
    {
    }

    endpoint::endpoint( const endpoint& a_orig ) :
            f_name( a_orig.f_name ),
            f_service( a_orig.f_service ),
            f_lockout_tag( a_orig.f_lockout_tag ),
            f_lockout_key( a_orig.f_lockout_key )
    {}

    endpoint::endpoint( endpoint&& a_orig ) :
            f_name( std::move(a_orig.f_name) ),
            f_service( std::move(a_orig.f_service) ),
            f_lockout_tag( std::move(a_orig.f_lockout_tag) ),
            f_lockout_key( std::move(a_orig.f_lockout_key) )
    {}

    endpoint::~endpoint()
    {}

    endpoint& endpoint::operator=( const endpoint& a_orig )
    {
        f_name = a_orig.f_name;
        f_service = a_orig.f_service;
        f_lockout_tag = a_orig.f_lockout_tag;
        f_lockout_key = a_orig.f_lockout_key;
        return *this;
    }

    endpoint& endpoint::operator=( endpoint&& a_orig )
    {
        f_name = std::move(a_orig.f_name);
        f_service = std::move(a_orig.f_service);
        f_lockout_tag = std::move(a_orig.f_lockout_tag);
        f_lockout_key = std::move(a_orig.f_lockout_key);
        return *this;
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
        // reply object to store whatever reply we end up with
        reply_ptr_t t_reply;

        // lambda to send the reply.  this local function is defined so we can send from within the catch block if needed before rethrowing.
        auto t_replier = [&t_reply, &a_request, this](){
            // send the reply if the request had a reply-to
            if( a_request->reply_to().empty() )
            {
                LWARN( dlog, "Not sending reply (reply-to empty)\n" <<
                            "    Return code: " << t_reply->get_return_code() << '\n' <<
                            "    Return message: " << t_reply->return_message() << '\n' <<
                            "    Payload:\n" << t_reply->payload() );
            }
            else
            {
                send_reply( t_reply );
            }
        };

        try
        {
            if( ! a_request->get_is_valid() )
            {
                std::string t_message( "Request message was not valid" );
                // check in the payload for error information
                if( a_request->payload().is_node() )
                {
                    const scarab::param_node& t_payload = a_request->payload().as_node();
                    if( t_payload.has("error") ) t_message += "; " + t_payload["error"]().as_string();
                }
                throw throw_reply( dl_message_error_decoding_fail{}, a_request->get_payload_ptr()->clone() ) << "Request message was not valid";
            }

            // the lockout key must be valid
            if( ! a_request->get_lockout_key_valid() )
            {
                throw throw_reply( dl_message_error_invalid_key{} ) << "Lockout key could not be parsed";
            }

            switch( a_request->get_message_operation() )
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
                    throw throw_reply( dl_message_error_invalid_method() ) << "Unrecognized message operation: <" << a_request->get_message_type() << ">";
                    break;
            } // end switch on message type
            // reply to be sent outside the try block
        }
        catch( const throw_reply& e )
        {
            if( e.ret_code().rc_value() == dl_success::s_value )
            {
                LINFO( dlog, "Replying with: " << e.return_message() );
            }
            else
            {
                LWARN( dlog, "Replying with: " << e.return_message() );
            }
            t_reply = a_request->reply( e.ret_code(), e.return_message() );
            t_reply->set_payload( e.get_payload_ptr()->clone() );
            // don't rethrow a throw_reply
            // reply to be sent outside the catch block
        }
#ifdef DL_PYTHON
        catch( const pybind11::error_already_set& e )
        {
            // check whether the error message from python starts with the keyword
            // the keyword should be the name of the python class
            if( std::string(e.what()).substr(0, throw_reply::py_throw_reply_keyword().size()) == throw_reply::py_throw_reply_keyword() )
            {
                reply_cache* t_reply_cache = reply_cache::get_instance();
                if( t_reply_cache->ret_code().rc_value() == dl_success::s_value )
                {
                    LINFO( dlog, "Replying with: " << t_reply_cache->return_message() );
                }
                else
                {
                    LWARN( dlog, "Replying with: " << t_reply_cache->return_message() );
                }
                t_reply = a_request->reply( t_reply_cache->ret_code(),t_reply_cache->return_message() );
                t_reply->set_payload( t_reply_cache->get_payload_ptr()->clone() );
                // don't rethrow a throw_reply
                // reply to be sent outside the catch block
            }
            else
            {
                // treat the python exception as a standard exception
                LERROR( dlog, "Caught exception from Python: " << e.what() );
                t_reply = a_request->reply( dl_unhandled_exception(), e.what() );
                t_replier(); // send the reply before rethrowing
                throw; // unhandled exceptions should rethrow because they're by definition unhandled
            }
        }
#endif
        catch( const std::exception& e )
        {
            LERROR( dlog, "Caught exception: " << e.what() );
            t_reply = a_request->reply( dl_unhandled_exception(), e.what() );
            t_replier(); // send the reply before rethrowing
            throw; // unhandled exceptions should rethrow because they're by definition unhandled
        }
        
        // send the reply
        t_replier();

        return t_reply;
    }

    void endpoint::sort_message( message_ptr_t a_message )
    {
        if( a_message->is_request() )
        {
            on_request_message( std::static_pointer_cast< msg_request >( a_message ) );
        }
        else if( a_message->is_alert() )
        {
            on_alert_message( std::static_pointer_cast< msg_alert >( a_message ) );
        }
        else if( a_message->is_reply() )
        {
            on_reply_message( std::static_pointer_cast< msg_reply >( a_message ) );
        }
        else
        {
            throw dripline_error() << "Unknown message type";
        }
    }

    void endpoint::send_reply( reply_ptr_t a_reply ) const
    {
        if( ! f_service )
        {
            LWARN( dlog, "Cannot send reply because the service pointer is not set" );
            return;
        }

        LDEBUG( dlog, "Sending reply message to <" << a_reply->routing_key() << ">:\n" <<
                 "    Return code: " << a_reply->get_return_code() << '\n' <<
                 "    Return message: " << a_reply->return_message() << '\n' <<
                 "    Payload:\n" << a_reply->payload() );

        sent_msg_pkg_ptr t_receive_reply;
        try
        {
            t_receive_reply = f_service->send( a_reply );
        }
        catch( message_ptr_t )
        {
            LWARN( dlog, "Operating in offline mode; message not sent" );
            return;
        }
        catch( connection_error& e )
        {
            LERROR( dlog, "Unable to connect to the broker:\n" << e.what() );
            return;
        }
        catch( dripline_error& e )
        {
            LERROR( dlog, "Dripline error while sending reply:\n" << e.what() );
            return;
        }

        if( ! t_receive_reply->f_successful_send )
        {
            LERROR( dlog, "Failed to send reply:\n" + t_receive_reply->f_send_error_message );
            return;
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

        if( ! authenticate( a_request->lockout_key() ) )
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
            return handle_is_locked_request( a_request );
        }

        return do_get_request( a_request );
    }

    reply_ptr_t endpoint::__do_set_request( const request_ptr_t a_request )
    {
        LDEBUG( dlog, "Set request received" );

        if( ! authenticate( a_request->lockout_key() ) )
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
        if( ! authenticate( a_request->lockout_key() ) && t_instruction != "unlock" && t_instruction != "ping" && t_instruction != "set_condition" )
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
            return handle_lock_request( a_request );
        }
        else if( t_instruction == "unlock" )
        {
            a_request->parsed_specifier().pop_front();
            return handle_unlock_request( a_request );
        }
        else if( t_instruction == "ping" )
        {
            a_request->parsed_specifier().pop_front();
            return handle_ping_request( a_request );
        }
        else if( t_instruction == "set_condition" )
        {
            a_request->parsed_specifier().pop_front();
            return handle_set_condition_request( a_request );
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
        uuid_t t_new_key = enable_lockout( a_request->get_sender_info(), a_request->lockout_key() );
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
        return a_request->reply( dl_success(), "Hello, " + a_request->sender_exe() );
    }

} /* namespace dripline */
