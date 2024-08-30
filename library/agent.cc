/*
 * agent.cc
 *
 *  Created on: Jun 2, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "agent.hh"

#include "agent_config.hh"
#include "core.hh"
#include "dripline_constants.hh"
#include "dripline_exceptions.hh"
#include "dripline_version.hh"
#include "receiver.hh"
#include "uuid.hh"

#include "logger.hh"
#include "param_codec.hh"
#include "path.hh"
#include "signal_handler.hh"

#include <algorithm> // for min
#include <string>

// In Windows there's a preprocessor macro called uuid_t that conflicts with this typdef
#ifdef uuid_t
#undef uuid_t
#endif

using scarab::param;
using scarab::param_array;
using scarab::param_node;
using scarab::param_ptr_t;
using scarab::param_value;

namespace dripline
{
    LOGGER( dlog, "agent" );

    agent::agent() :
            f_is_dry_run( false ),
            f_routing_key(),
            f_specifier(),
            f_lockout_key( generate_nil_uuid() ),
            f_return_code( dl_success().rc_value() ),
            f_return_message(),
            f_timeout( 0 ),
            f_suppress_output( false ),
            f_json_print( false ),
            f_pretty_print( false ),
            f_save_filename(),
            f_reply(),
            f_return( dl_client_error().rc_value() )
    {
    }

    void agent::sub_agent::execute( const scarab::param_node& a_config, const scarab::authentication& a_auth )
    {
        const scarab::param_array a_ord_args;
        execute( a_config, a_ord_args, a_auth );
    }

    void agent::sub_agent::execute( const scarab::param_node& a_config, const scarab::param_array& a_ord_args, const scarab::authentication& a_auth )
    {
        LINFO( dlog, "Creating message" );

        // create a copy of the config that will be pared down by removing expected elements
        param_node t_config( a_config );

        param_node t_dripline_node;
        if( t_config.has( "dripline_mesh" ) )
        {
            t_dripline_node = std::move(t_config.remove( "dripline_mesh" )->as_node());
        }

        core t_core( t_dripline_node, a_auth );

        t_config.remove( "auth_file" );
        t_config.remove( "auth_groups" );

        f_agent->set_timeout( t_config.get_value( "timeout", 10U ) * 1000 ); // convert seconds (dripline agent user interface) to milliseconds (expected by SimpleAmqpClient)
        t_config.erase( "timeout" );
        f_agent->set_json_print( t_config.get_value( "json_print", f_agent->get_json_print() ) );
        t_config.erase( "json_print" );
        f_agent->set_pretty_print( t_config.get_value( "pretty_print", f_agent->get_pretty_print() ) );
        t_config.erase( "pretty_print" );
        f_agent->set_suppress_output( t_config.get_value( "suppress_output", f_agent->get_suppress_output() ) );
        t_config.erase( "suppress_output" );

        f_agent->routing_key() = t_config.get_value( "rk", f_agent->routing_key() );
        t_config.erase( "rk" );

        f_agent->specifier() = t_config.get_value( "specifier", f_agent->specifier() );
        t_config.erase( "specifier" );

        if( t_config.has( "lockout_key" ) )
        {
            bool t_lk_valid = true;
            f_agent->lockout_key() = dripline::uuid_from_string( t_config["lockout_key"]().as_string(), t_lk_valid );
            t_config.erase( "lockout_key" );
            if( ! t_lk_valid )
            {
                LERROR( dlog, "Invalid lockout key provided: <" << t_config.get_value( "lockout_key", "" ) << ">" );
                f_agent->set_return( dl_client_error().rc_value() );
                return;
            }
        }

        if( t_config.has( "return" ) )
        {
            f_agent->set_return_code( t_config["return"].as_node().get_value( "code", dl_success().rc_value() ) );
            f_agent->return_message() = t_config["return"].as_node().get_value( "message", "" );
            t_config.erase( "return" );
        }

        f_agent->save_filename() = t_config.get_value( "save", "" );
        t_config.erase( "save" );

        // load the values array, merged in the proper order
        scarab::param_array t_values;
        if( t_config.has( "values" ) )
        {
            t_values.merge( t_config["values"].as_array() );
            t_config.erase( "values" );
        }
        t_values.merge( a_ord_args );
        if( t_config.has( "option_values" ) )
        {
            t_values.merge( t_config["option_values"].as_array() );
            t_config.erase( "option_values" );
        }
        if( ! t_values.empty() )
        {
            t_config.add( "values", t_values );
        }
        
        // check if this is meant to be a dry run message
        if( t_config.has( "dry_run_msg" ) )
        {
            t_config.erase( "dry_run_msg" );
            f_agent->set_is_dry_run( true );
        }

        this->create_and_send_message( t_config, t_core );

        return;
    }

    void agent::sub_agent_request::create_and_send_message( scarab::param_node& a_config, const core& a_core )
    {
        // create the request
        request_ptr_t t_request = this->create_request( a_config );
        LDEBUG( dlog, "message payload to send is: " << t_request->payload() );

        if( ! t_request )
        {
            LERROR( dlog, "Unable to create request" );
            f_agent->set_return( dl_client_error_invalid_request().rc_value() );
            return;
        }

        // if this is a dry run, we print the message and stop here
        if( f_agent->get_is_dry_run() )
        {
            LPROG( dlog, "Request (routing key = " << f_agent->routing_key() << ";  specifier = " << f_agent->specifier() << "):\n" << *t_request );
            f_agent->set_return( dl_warning_dry_run().rc_value() );
            return;
        }

        // now all that remains in f_config should be values to pass to the server as arguments to the request

        t_request->lockout_key() = f_agent->lockout_key();

        LINFO( dlog, "Sending message w/ message_operation = " << t_request->get_message_operation() << " to " << t_request->routing_key() );
        LDEBUG( dlog, "Message headers:\n" << t_request->get_message_param( false ) );

        sent_msg_pkg_ptr t_receive_reply;
        try
        {
            t_receive_reply = a_core.send( t_request );
        }
        catch( message_ptr_t )
        {
            LWARN( dlog, "Operating in offline mode; message not sent" );
            f_agent->set_return( dl_warning_offline().rc_value() );
            return;
        }
        catch( connection_error& e )
        {
            LERROR( dlog, "Unable to connect to the broker:\n" << e.what() );
            f_agent->set_return( dl_amqp_error_broker_connection().rc_value() );
            return;
        }
        catch( dripline_error& e )
        {
            LERROR( dlog, "Unable to send request:\n" << e.what() );
            f_agent->set_return( dl_client_error_unable_to_send().rc_value() );
            return;
        }

        if( ! t_receive_reply->f_successful_send )
        {
            LERROR( dlog, "Unable to send request:\n" + t_receive_reply->f_send_error_message );
            f_agent->set_return( dl_client_error_unable_to_send().rc_value() );
            return;
        }

        if( ! t_receive_reply->f_consumer_tag.empty() )  // this indicates that the reply queue was created, and we've started consuming on it; we should wait for a reply
        {
            LINFO( dlog, "Waiting for a reply from the server; use ctrl-c to cancel" );

            // timed blocking call to wait for incoming message
            receiver t_msg_receiver;
            core::post_listen_status t_post_listen_status = core::post_listen_status::unknown;
            auto t_rec_cancel_wrap = wrap_cancelable( t_msg_receiver );
            scarab::signal_handler::add_cancelable( t_rec_cancel_wrap );
            dripline::reply_ptr_t t_reply = t_msg_receiver.wait_for_reply( t_receive_reply, t_post_listen_status, f_agent->get_timeout() );

            if( t_msg_receiver.is_canceled() )
            {
                LDEBUG( dlog, "Agent canceled while waiting for reply" );
                f_agent->set_return( dl_success().rc_value() );
            }
            else if( t_reply )
            {
                LINFO( dlog, "Response received" );
                f_agent->set_return( t_reply->get_return_code() );

                const param& t_payload = t_reply->payload();

                LPROG( dlog, "Response:\n" <<
                        "Return code: " << t_reply->get_return_code() << '\n' <<
                        "Return message: " << t_reply->return_message() << '\n' <<
                        t_payload );

                if( ! f_agent->get_suppress_output() )
                {
                    if( ! f_agent->get_json_print() && ! f_agent->get_pretty_print() )
                    {
                        std::cout << *t_reply << std::endl;
                    }
                    else
                    {
                        param_node t_encoding_options;
                        if( f_agent->get_pretty_print() )
                        {
                            t_encoding_options.add( "style", "pretty" );
                        }
                        std::string t_encoded_message = t_reply->encode_full_message( 5000, t_encoding_options );
                        std::cout << t_encoded_message << std::endl;
                    }
                }

                if( ! f_agent->save_filename().empty() && ! t_payload.is_null() )
                {
                    scarab::param_translator t_translator;
                    if( ! t_translator.write_file( t_payload, f_agent->save_filename() ) )
                    {
                        LERROR( dlog, "Unable to write out payload" );
                        f_agent->set_return( dl_client_error_handling_reply().rc_value() );
                    }
                }
            }
            else
            {
                if( t_post_listen_status == core::post_listen_status::timeout )
                {
                    LWARN( dlog, "Timed out or while waiting for reply" );
                    f_agent->set_return( dl_client_error_timeout().rc_value() );
                }
                else
                {
                    if( t_post_listen_status == core::post_listen_status::hard_error ) 
                    {
                        LERROR( dlog, "Error while waiting for reply" );
                    }
                    else 
                    {
                        LERROR( dlog, "Unknown state while waiting for reply: " << (int)t_post_listen_status );
                    }
                    f_agent->set_return( dl_client_error().rc_value() );
                }
            }
            f_agent->set_reply( t_reply );
        }
        else
        {
            f_agent->set_return( dl_success().rc_value() );
        }

        return;
    }

    void agent::sub_agent_reply::create_and_send_message( scarab::param_node& a_config, const core& a_core )
    {
        // create the alert
        param_ptr_t t_payload_ptr( new param_node( a_config ) );

        reply_ptr_t t_reply =  msg_reply::create( f_agent->get_return_code(),
                                                  f_agent->return_message(),
                                                  std::move(t_payload_ptr),
                                                  f_agent->routing_key(),
                                                  f_agent->specifier() );
        LDEBUG( dlog, "reply payload to send is: " << t_reply->payload() );

        if( ! t_reply )
        {
            LERROR( dlog, "Unable to create reply" );
            f_agent->set_return( dl_client_error_invalid_request().rc_value() );
            return;
        }

        // if this is a dry run, we print the message and stop here
        if( f_agent->get_is_dry_run() )
        {
            LPROG( dlog, "Reply (routing key = " << f_agent->routing_key() << ";  specifier = " << f_agent->specifier() << "):\n" << *t_reply );
            f_agent->set_return( dl_warning_dry_run().rc_value() );
            return;
        }

        LINFO( dlog, "Sending reply with return code <" << t_reply->get_return_code() << "> and message <" << t_reply->return_message() << "> to key " << t_reply->routing_key() );
        LDEBUG( dlog, "Message headers:\n" << t_reply->get_message_param( false ) );

        sent_msg_pkg_ptr t_msg_sent;
        try
        {
            t_msg_sent = a_core.send( t_reply );
        }
        catch( message_ptr_t )
        {
            LWARN( dlog, "Operating in offline mode; message not sent" );
            f_agent->set_return( dl_warning_offline().rc_value() );
            return;
        }
        catch( connection_error& e )
        {
            LERROR( dlog, "Unable to connect to the broker:\n" << e.what() );
            f_agent->set_return( dl_amqp_error_broker_connection().rc_value() );
            return;
        }
        catch( dripline_error& e )
        {
            LERROR( dlog, "Unable to send reply:\n" << e.what() );
            f_agent->set_return( dl_client_error_unable_to_send().rc_value() );
            return;
        }

        if( ! t_msg_sent->f_successful_send )
        {
            LERROR( dlog, "Unable to send reply:\n" + t_msg_sent->f_send_error_message );
            f_agent->set_return( dl_client_error_unable_to_send().rc_value() );
        }
        else
        {
            f_agent->set_return( dl_success().rc_value() );
        }

        return;
    }

    void agent::sub_agent_alert::create_and_send_message( scarab::param_node& a_config, const core& a_core )
    {
        // create the alert
        param_ptr_t t_payload_ptr( new param_node( a_config ) );

        alert_ptr_t t_alert =  msg_alert::create( std::move(t_payload_ptr),
                                                  f_agent->routing_key(),
                                                  f_agent->specifier() );
        LDEBUG( dlog, "alert payload to send is: " << t_alert->payload() );

        if( ! t_alert )
        {
            LERROR( dlog, "Unable to create alert" );
            f_agent->set_return( dl_client_error_invalid_request().rc_value() );
            return;
        }

        // if this is a dry run, we print the message and stop here
        if( f_agent->get_is_dry_run() )
        {
            LPROG( dlog, "Alert (routing key = " << f_agent->routing_key() << ";  specifier = " << f_agent->specifier() << "):\n" << *t_alert );
            f_agent->set_return( dl_warning_dry_run().rc_value() );
            return;
        }

        LINFO( dlog, "Sending alert with key " << t_alert->routing_key() );
        LDEBUG( dlog, "Message headers:\n" << t_alert->get_message_param( false ) );

        sent_msg_pkg_ptr t_msg_sent;
        try
        {
            t_msg_sent = a_core.send( t_alert );
        }
        catch( message_ptr_t )
        {
            LWARN( dlog, "Operating in offline mode; message not sent" );
            f_agent->set_return( dl_warning_offline().rc_value() );
            return;
        }
        catch( connection_error& e )
        {
            LERROR( dlog, "Unable to connect to the broker:\n" << e.what() );
            f_agent->set_return( dl_amqp_error_broker_connection().rc_value() );
            return;
        }
        catch( dripline_error& e )
        {
            LERROR( dlog, "Unable to send alert:\n" << e.what() );
            f_agent->set_return( dl_client_error_unable_to_send().rc_value() );
            return;
        }

        if( ! t_msg_sent->f_successful_send )
        {
            LERROR( dlog, "Unable to send alert:\n" + t_msg_sent->f_send_error_message );
            f_agent->set_return( dl_client_error_unable_to_send().rc_value() );
        }
        else
        {
            f_agent->set_return( dl_success().rc_value() );
        }

        return;
    }

    request_ptr_t agent::sub_agent_get::create_request( scarab::param_node& a_config )
    {
        param_ptr_t t_payload_ptr( new param_node( a_config ) );

        return msg_request::create( std::move(t_payload_ptr),
                                    op_t::get,
                                    f_agent->routing_key(),
                                    f_agent->specifier() );
    }

    request_ptr_t agent::sub_agent_set::create_request( scarab::param_node& a_config )
    {
        // require the values array
        if( ! a_config.has( "values" ) )
        {
            LERROR( dlog, "No \"values\" option given" );
            return nullptr;
        }

        param_ptr_t t_payload_ptr( new param_node( a_config ) );

        return msg_request::create( std::move(t_payload_ptr),
                                    op_t::set,
                                    f_agent->routing_key(),
                                    f_agent->specifier() );
    }

    request_ptr_t agent::sub_agent_cmd::create_request( param_node& a_config )
    {
        param_ptr_t t_payload_ptr( new param_node() );
        param_node& t_payload_node = t_payload_ptr->as_node();

        // for the load instruction, the instruction node should be replaced by the contents of the file specified
        if( a_config.has( "load" ) )
        {
            if( ! a_config["load"].as_node().has( "json" ) )
            {
                LERROR( dlog, "Load instruction did not contain a valid file type");
                return nullptr;
            }

            std::string t_load_filename( a_config["load"]().as_string() );
            scarab::param_translator t_translator;
            scarab::param_ptr_t t_node_from_file = t_translator.read_file( t_load_filename );
            if( t_node_from_file == nullptr || ! t_node_from_file->is_node() )
            {
                LERROR( dlog, "Unable to read JSON file <" << t_load_filename << ">" );
                return nullptr;
            }

            t_payload_node.merge( t_node_from_file->as_node() );
            a_config.erase( "load" );
        }

        // at this point, all that remains in a_config should be other options that we want to add to the payload node
        t_payload_node.merge( a_config );

        return msg_request::create( std::move(t_payload_ptr),
                                    op_t::cmd,
                                    f_agent->routing_key(),
                                    f_agent->specifier() );
    }

} /* namespace dripline */
