/*
 * service.cc
 *
 *  Created on: Jan 5, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "service.hh"

#include "dripline_error.hh"

#include "authentication.hh"
#include "logger.hh"

using scarab::authentication;
using scarab::param_node;
using scarab::param_value;
using scarab::param_ptr_t;

using std::static_pointer_cast;
using std::string;
using std::set;

namespace dripline
{
    LOGGER( dlog, "service" );

    service::service( const scarab::param_node& a_config, const string& a_queue_name,  const std::string& a_broker_address, unsigned a_port, const std::string& a_auth_file, const bool a_make_connection ) :
            core( a_config, a_broker_address, a_port, a_auth_file, a_make_connection ),
            // logic for setting the name:
            //   a_queue_name if provided
            //   otherwise a_config["queue"] if it exists
            //   otherwise "dlcpp_service"
            endpoint( a_queue_name.empty() ? a_config.get_value( "queue", "dlcpp_service" ) : a_queue_name, *this ),
            cancelable(),
            f_channel(),
            f_consumer_tag(),
            f_keys(),
            f_broadcast_key( "broadcast" ),
            f_listen_timeout_ms( 500 ),
            f_single_message_wait_ms( 1000 ),
            f_msg_receiver(),
            f_lockout_tag(),
            f_lockout_key( generate_nil_uuid() )
    {
        // get values from the config
        f_listen_timeout_ms = a_config.get_value( "listen-timeout-ms", f_listen_timeout_ms );

        // override if specified as a separate argument
        if( ! a_queue_name.empty() ) f_name = a_queue_name;
    }

    service::service( const bool a_make_connection, const scarab::param_node& a_config ) :
            core( a_make_connection, a_config ),
            endpoint( "", *this ),
            cancelable(),
            f_channel(),
            f_consumer_tag(),
            f_keys(),
            f_broadcast_key(),
            f_listen_timeout_ms( 500 ),
            f_single_message_wait_ms( 1000 ),
            f_msg_receiver(),
            f_lockout_tag(),
            f_lockout_key( generate_nil_uuid() )
    {
    }

    service::~service()
    {
    }

    sent_msg_pkg_ptr service::send( request_ptr_t a_request ) const
    {
        a_request->sender_service_name() = f_name;
        return core::send( a_request );
    }

    sent_msg_pkg_ptr service::send( reply_ptr_t a_reply ) const
    {
        a_reply->sender_service_name() = f_name ;
        return core::send( a_reply );
    }

    sent_msg_pkg_ptr service::send( alert_ptr_t a_alert ) const
    {
        a_alert->sender_service_name() = f_name;
        return core::send( a_alert );
    }

    bool service::start()
    {
        if( ! f_make_connection )
        {
            LWARN( dlog, "Should not start service when make_connection is disabled" );
            return true;
        }
        if( f_name.empty() )
        {
            LERROR( dlog, "Service requires a queue name to be started" );
            return false;
        }

        LINFO( dlog, "Connecting to <" << f_address << ":" << f_port << ">" );

        f_channel = open_channel();
        if( ! f_channel ) return false;

        if( ! setup_exchange( f_channel, f_requests_exchange ) ) return false;
        if( ! setup_exchange( f_channel, f_alerts_exchange ) ) return false;

        if( ! setup_queue( f_channel, f_name ) ) return false;

        if( ! bind_keys( f_keys ) ) return false;

        if( ! start_consuming() ) return false;

        f_canceled.store( false );

        return true;
    }

    bool service::listen()
    {
        LINFO( dlog, "Listening for incoming messages on <" << f_name << ">" );

        if ( ! f_make_connection )
        {
            return true;
        }
        while( ! f_canceled.load()  )
        {

            amqp_envelope_ptr t_envelope;
            bool t_channel_valid = listen_for_message( t_envelope, f_channel, f_consumer_tag, f_listen_timeout_ms );

            if( f_canceled.load() )
            {
                LDEBUG( dlog, "Service canceled" );
                return true;
            }

            if( ! t_envelope && t_channel_valid )
            {
                continue;
            }

            amqp_message_ptr t_message = t_envelope->Message();

            auto t_parsed_message_id = message::parse_message_id( t_message->MessageId() );
            if( f_msg_receiver.incoming_messages().count( std::get<0>(t_parsed_message_id) ) == 0 )
            {
                // this path: first chunk for this message
                // create the new message_pack object
                incoming_message_pack& t_pack = f_msg_receiver.incoming_messages()[std::get<0>(t_parsed_message_id)];
                // set the f_messages vector to the expected size
                t_pack.f_messages.resize( std::get<2>(t_parsed_message_id) );
                // put in place the first message chunk received
                t_pack.f_messages[std::get<1>(t_parsed_message_id)] = t_message;
                t_pack.f_routing_key = t_envelope->RoutingKey();

                // start the thread to wait for message chunks
                t_pack.f_thread = std::thread([this, &t_pack, &t_parsed_message_id](){ wait_for_message(t_pack, std::get<0>(t_parsed_message_id)); });
                t_pack.f_thread.detach();
            }
            else
            {
                // this path: have already received chunks from this message
                incoming_message_pack& t_pack = f_msg_receiver.incoming_messages()[std::get<0>(t_parsed_message_id)];
                if( t_pack.f_processing.load() )
                {
                    LWARN( dlog, "Message <" << std::get<0>(t_parsed_message_id) << "> is already being processed\n" <<
                            "Just received chunk " << std::get<1>(t_parsed_message_id) << " of " << std::get<2>(t_parsed_message_id) );
                }
                else
                {
                    // lock mutex to access f_messages
                    std::unique_lock< std::mutex > t_lock( t_pack.f_mutex );
                    if( t_pack.f_messages[std::get<1>(t_parsed_message_id)] )
                    {
                        LWARN( dlog, "Received duplicate message chunk for message <" << std::get<0>(t_parsed_message_id) << ">; chunk " << std::get<1>(t_parsed_message_id) );
                    }
                    else
                    {
                        // add chunk to set of chunks
                        t_pack.f_messages[std::get<1>(t_parsed_message_id)] = t_message;
                        ++t_pack.f_chunks_received;
                        t_lock.unlock();
                        // inform the message-processing thread it should check whether it has the complete message
                        t_pack.f_conv.notify_one();
                    }
                }
            }


            if( ! t_channel_valid )
            {
                LERROR( dlog, "Channel is no longer valid" );
                return false;
            }

            if( f_canceled.load() )
            {
                LDEBUG( dlog, "Service canceled" );
                return true;
            }
        }
        return true;
    }

    bool service::stop()
    {
        LINFO( dlog, "Stopping service on <" << f_name << ">" );

        if( ! stop_consuming() ) return false;

        if( ! remove_queue() ) return false;

        return true;
    }


    bool service::bind_keys( const set< string >& a_keys )
    {
#ifdef DL_OFFLINE
        return false;
#endif

        try
        {
            for( set< string >::const_iterator t_key_it = a_keys.begin(); t_key_it != a_keys.end(); ++t_key_it )
            {
                f_channel->BindQueue( f_name, f_requests_exchange, *t_key_it );
            }
            f_channel->BindQueue( f_name, f_requests_exchange, f_name + ".#" );
            f_channel->BindQueue( f_name, f_requests_exchange, f_broadcast_key + ".#" );
            return true;
        }
        catch( amqp_exception& e )
        {
            LERROR( dlog, "AMQP exception caught while declaring binding keys: (" << e.reply_code() << ") " << e.reply_text() );
            return false;
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "AMQP library exception caught while binding keys: (" << e.ErrorCode() << ") " << e.what() );
            return false;
        }
    }

    bool service::start_consuming()
    {
#ifdef DL_OFFLINE
        return false;
#endif

        try
        {
            LDEBUG( dlog, "Starting to consume messages" );
            // second bool is setting no_ack to false
            f_consumer_tag = f_channel->BasicConsume( f_name, "", true, false );
            return true;
        }
        catch( amqp_exception& e )
        {
            LERROR( dlog, "AMQP exception caught while starting consuming messages: (" << e.reply_code() << ") " << e.reply_text() );
            return false;
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "AMQP library exception caught while starting consuming messages: (" << e.ErrorCode() << ") " << e.what() );
            return false;
        }
    }

    bool service::stop_consuming()
    {
#ifdef DL_OFFLINE
        return false;
#endif

        if ( ! f_make_connection )
        {
            LDEBUG( dlog, "no consuming to start because connections disabled" );
            return true;
        }
        try
        {
            LDEBUG( dlog, "Stopping consuming messages (consumer " << f_consumer_tag << ")" );
            f_channel->BasicCancel( f_consumer_tag );
            f_consumer_tag.clear();
            return true;
        }
        catch( amqp_exception& e )
        {
            LERROR( dlog, "AMQP exception caught while stopping consuming messages: (" << e.reply_code() << ") " << e.reply_text() );
            return false;
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "AMQP library exception caught while stopping consuming messages: (" << e.ErrorCode() << ") " << e.what() );
            return false;
        }
        catch( AmqpClient::ConsumerTagNotFoundException& e )
        {
            LERROR( dlog, "Fatal AMQP exception encountered: " << e.what() );
            return false;
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Standard exception caught: " << e.what() );
            return false;
        }
        catch(...)
        {
            LERROR( dlog, "Unknown exception caught" );
            return false;
        }
    }

    bool service::remove_queue()
    {
#ifdef DL_OFFLINE
        return false;
#endif
        if ( ! f_make_connection )
        {
            LDEBUG( dlog, "no queue to remove because make_connection is false" );
            return true;
        }
        try
        {
            LDEBUG( dlog, "Deleting queue <" << f_name << ">" );
            f_channel->DeleteQueue( f_name, false );
            f_name.clear();
        }
        catch( AmqpClient::ConnectionClosedException& e )
        {
            LERROR( dlog, "Fatal AMQP exception encountered: " << e.what() );
            return false;
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "AMQP library exception caught while removing queue: (" << e.ErrorCode() << ") " << e.what() );
            return false;
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Standard exception caught: " << e.what() );
            return false;
        }
        catch(...)
        {
            LERROR( dlog, "Unknown exception caught" );
            return false;
        }

        return true;
    }

    void service::wait_for_message( incoming_message_pack& a_pack, const std::string& a_message_id )
    {
        std::unique_lock< std::mutex > t_lock( a_pack.f_mutex );

        if( a_pack.f_chunks_received == a_pack.f_messages.size() )
        {
            t_lock.release(); // process_message() will unlock the mutex before erasing the message pack
            process_message( a_pack, a_message_id );
            return;
        }

        auto t_now = std::chrono::system_clock::now();

        while( a_pack.f_conv.wait_until( t_lock, t_now + std::chrono::milliseconds(f_single_message_wait_ms) ) == std::cv_status::no_timeout )
        {
            if( a_pack.f_chunks_received == a_pack.f_messages.size() )
            {
                t_lock.release(); // process_message() will unlock the mutex before erasing the message pack
                process_message( a_pack, a_message_id );
                return;
            }
        }

        t_lock.release(); // process_message() will unlock the mutex before erasing the message pack
        process_message( a_pack, a_message_id );

        return;
    }

    void service::process_message( incoming_message_pack& a_pack, const std::string& a_message_id )
    {
        a_pack.f_processing.store( true );
        try
        {
            message_ptr_t t_message = message::process_message( a_pack.f_messages, a_pack.f_routing_key );

            a_pack.f_mutex.unlock();
            f_msg_receiver.incoming_messages().erase( a_message_id );

            bool t_msg_handled = true;
            if( t_message->is_request() )
            {
                on_request_message( static_pointer_cast< msg_request >( t_message ) );
            }
            else if( t_message->is_alert() )
            {
                on_alert_message( static_pointer_cast< msg_alert >( t_message ) );
            }
            else if( t_message->is_reply() )
            {
                on_reply_message( static_pointer_cast< msg_reply >( t_message ) );
            }
            if( ! t_msg_handled )
            {
                throw dripline_error() << "Message could not be handled";
            }
        }
        catch( dripline_error& e )
        {
            LERROR( dlog, "Dripline exception caught while handling message: " << e.what() );
        }
        catch( amqp_exception& e )
        {
            LERROR( dlog, "AMQP exception caught while sending reply: (" << e.reply_code() << ") " << e.reply_text() );
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "AMQP Library Exception caught while sending reply: (" << e.ErrorCode() << ") " << e.what() );
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Standard exception caught while sending reply: " << e.what() );
        }

        return;
    }

    uuid_t service::enable_lockout( const scarab::param_node& a_tag, uuid_t a_key )
    {
        if( is_locked() ) return generate_nil_uuid();
        if( a_key.is_nil() ) f_lockout_key = generate_random_uuid();
        else f_lockout_key = a_key;
        f_lockout_tag = a_tag;
        return f_lockout_key;
    }

    bool service::disable_lockout( const uuid_t& a_key, bool a_force )
    {
        if( ! is_locked() ) return true;
        if( ! a_force && a_key != f_lockout_key ) return false;
        f_lockout_key = generate_nil_uuid();
        f_lockout_tag.clear();
        return true;
    }

    bool service::authenticate( const uuid_t& a_key ) const
    {
        LDEBUG( dlog, "Authenticating with key <" << a_key << ">" );
        if( is_locked() ) return check_key( a_key );
        return true;
    }

    reply_ptr_t service::handle_lock_request( const request_ptr_t a_request )
    {
        uuid_t t_new_key = enable_lockout( a_request->get_sender_info(), a_request->lockout_key() );
        if( t_new_key.is_nil() )
        {
            return a_request->reply( dl_device_error(), "Unable to lock server" );;
        }

        param_ptr_t t_payload_ptr( new param_node() );
        param_node& t_payload_node = t_payload_ptr->as_node();
        t_payload_node.add( "key", string_from_uuid( t_new_key ) );
        return a_request->reply( dl_success(), "Server is now locked", std::move(t_payload_ptr) );
    }

    reply_ptr_t service::handle_unlock_request( const request_ptr_t a_request )
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

    reply_ptr_t service::handle_set_condition_request( const request_ptr_t a_request )
    {
        return this->__do_handle_set_condition_request( a_request );
    }

    reply_ptr_t service::handle_is_locked_request( const request_ptr_t a_request )
    {
        bool t_is_locked = is_locked();
        scarab::param_ptr_t t_reply_payload( new param_node() );
        scarab::param_node& t_reply_node = t_reply_payload->as_node();
        t_reply_node.add( "is_locked", t_is_locked );
        if( t_is_locked ) t_reply_node.add( "tag", f_lockout_tag );
        return a_request->reply( dl_success(), "Checked lock status", std::move(t_reply_payload) );
    }

} /* namespace dripline */
