/*
 * service.cc
 *
 *  Created on: Jan 5, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "service.hh"

#include "authentication.hh"
#include "logger.hh"

using scarab::authentication;
using scarab::param_node;
using scarab::param_value;
using scarab::param_ptr_t;

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
            endpoint( a_queue_name.empty() ? a_config.get_value( "queue", "dlcpp_service" ) : a_queue_name ),
            listener(),
            receiver_parallel(),
            std::enable_shared_from_this< service >(),
            f_status( status::nothing ),
            f_single_message_wait_ms( 1000 ),
            f_sync_children(),
            f_async_children(),
            f_broadcast_key( "broadcast" )
    {
        // get values from the config
        f_listen_timeout_ms = a_config.get_value( "listen-timeout-ms", f_listen_timeout_ms );
        f_single_message_wait_ms = a_config.get_value( "message-wait-ms", f_single_message_wait_ms );

        // override if specified as a separate argument
        if( ! a_queue_name.empty() ) f_name = a_queue_name;
    }

    service::service( const bool a_make_connection, const scarab::param_node& a_config ) :
            core( a_make_connection, a_config ),
            endpoint( "" ),
            listener(),
            receiver_parallel(),
            std::enable_shared_from_this< service >(),
            f_status( status::nothing ),
            f_single_message_wait_ms( 1000 ),
            f_sync_children(),
            f_async_children(),
            f_broadcast_key()
    {
    }

    service::~service()
    {
        if( f_status > status::exchange_declared ) stop();
    }

    bool service::add_child( endpoint_ptr_t a_endpoint_ptr )
    {
        auto t_inserted = f_sync_children.insert( std::make_pair( a_endpoint_ptr->name(), a_endpoint_ptr ) );
        if( t_inserted.second )
        {
            try
            {
                a_endpoint_ptr->service() = shared_from_this();
            }
            catch( std::bad_weak_ptr& e )
            {
                LWARN( dlog, "add_child called from service constructor (or for some other reason the shared-pointer is bad); Service pointer not set.");
            }
        }
        return t_inserted.second;
    }

    bool service::add_async_child( endpoint_ptr_t a_endpoint_ptr )
    {
        listener_ptr_t t_listener_ptr = std::dynamic_pointer_cast< listener >( a_endpoint_ptr );
        if( ! t_listener_ptr )
        {
            t_listener_ptr.reset( new listener_endpoint(a_endpoint_ptr) );
        }
        auto t_inserted = f_async_children.insert( std::make_pair( a_endpoint_ptr->name(), t_listener_ptr ) );
        if( t_inserted.second )
        {
            try
            {
                a_endpoint_ptr->service() = shared_from_this();
            }
            catch( std::bad_weak_ptr& e )
            {
                LWARN( dlog, "add_async_child called from service constructor (or for some other reason the shared-pointer is bad); Service pointer not set.");
            }
        }
        return t_inserted.second;
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

        // fill in the link to this in endpoint because we couldn't do it in the constructor
        endpoint::f_service = this->shared_from_this();

        LINFO( dlog, "Connecting to <" << f_address << ":" << f_port << ">" );

        if( ! open_channels() ) return false;
        f_status = status::channel_created;

        if( ! setup_exchange( f_channel, f_requests_exchange ) ) return false;
        if( ! setup_exchange( f_channel, f_alerts_exchange ) ) return false;
        f_status = status::exchange_declared;

        if( ! setup_queues() ) return false;
        f_status = status::queue_declared;

        if( ! bind_keys() ) return false;
        f_status = status::queue_bound;

        if( ! start_consuming() ) return false;
        f_status = status::consuming;

        f_canceled.store( false );

        return true;
    }

    bool service::listen()
    {
        if ( ! f_make_connection )
        {
            LWARN( dlog, "Should not listen for messages when make_connection is disabled" );
            return true;
        }

        f_status = status::listening;

        std::thread t_rec_thread( &receiver::execute, this );

        for( async_map_t::iterator t_child_it = f_async_children.begin();
                t_child_it != f_async_children.end();
                ++t_child_it )
        {
            t_child_it->second->thread() = std::thread( &listener::listen_on_queue, t_child_it->second.get() );
        }

        listen_on_queue();

        for( async_map_t::iterator t_child_it = f_async_children.begin();
                t_child_it != f_async_children.end();
                ++t_child_it )
        {
            t_child_it->second->thread().join();
        }

        t_rec_thread.join();

        return true;
    }

    bool service::stop()
    {
        LINFO( dlog, "Stopping service on <" << f_name << ">" );

        if( f_status >= status::listening ) // listening or processing
        {
            this->cancel( dl_success().rc_value() );
            f_status = status::consuming;
        }
        if( f_status >= status::queue_bound ) // queue_bound or consuming
        {
            if( ! stop_consuming() ) return false;
            f_status = status::queue_bound;
        }


        if( f_status >= status::queue_declared ) // queue_declared or queue_bound
        {
            if( ! remove_queue() ) return false;
            f_status = status::exchange_declared;
        }

        return true;
    }

    bool service::open_channels()
    {
#ifdef DL_OFFLINE
        return false;
#endif

        LDEBUG( dlog, "Opening channel for service <" << f_name << ">" );
        f_channel = open_channel();
        if( ! f_channel ) return false;

        for( async_map_t::iterator t_child_it = f_async_children.begin();
                t_child_it != f_async_children.end();
                ++t_child_it )
        {
            LDEBUG( dlog, "Opening channel for child <" << t_child_it->first << ">" );
            t_child_it->second->channel() = open_channel();
            t_child_it->second->set_listen_timeout_ms( f_listen_timeout_ms );
        }
        return true;
    }

    bool service::setup_queues()
    {
#ifdef DL_OFFLINE
        return false;
#endif

        LDEBUG( dlog, "Setting up queue for service <" << f_name << ">" );
        if( ! setup_queue( f_channel, f_name ) ) return false;

        for( async_map_t::iterator t_child_it = f_async_children.begin();
                t_child_it != f_async_children.end();
                ++t_child_it )
        {
            LDEBUG( dlog, "Setting up queue for async child <" << t_child_it->first << ">" );
            if( ! setup_queue( t_child_it->second->channel(), t_child_it->first ) ) return false;
        }

        return true;
    }

    bool service::bind_keys()
    {
#ifdef DL_OFFLINE
        return false;
#endif

        try
        {
            LDEBUG( dlog, "Binding key <" << f_name << ".#> to queue " << f_name );
            f_channel->BindQueue( f_name, f_requests_exchange, f_name + ".#" );
            LDEBUG( dlog, "Binding key <" << f_broadcast_key << ".#> to queue " << f_name );
            f_channel->BindQueue( f_name, f_requests_exchange, f_broadcast_key + ".#" );
            LDEBUG( dlog, "Binding keys for synchronous children" );
            for( sync_map_t::const_iterator t_child_it = f_sync_children.begin();
                    t_child_it != f_sync_children.end();
                    ++t_child_it )
            {
                LDEBUG( dlog, "Binding key <" << t_child_it->first << ".#> to queue " << f_name );
                f_channel->BindQueue( f_name, f_requests_exchange, t_child_it->first + ".#" );
            }

            LDEBUG( dlog, "Binding keys for asynchronous children" );
            for( async_map_t::iterator t_child_it = f_async_children.begin();
                    t_child_it != f_async_children.end();
                    ++t_child_it )
            {
                LDEBUG( dlog, "Binding key <" << t_child_it->first << ".#> to queue " << t_child_it->first );
                t_child_it->second->channel()->BindQueue( t_child_it->first, f_requests_exchange, t_child_it->first + ".#" );
            }
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
            LDEBUG( dlog, "Starting to consume messages on <" << f_name << ">" );
            // second bool is setting no_ack to false
            f_consumer_tag = f_channel->BasicConsume( f_name, "", true, false );

            for( async_map_t::iterator t_child_it = f_async_children.begin();
                    t_child_it != f_async_children.end();
                    ++t_child_it )
            {
                LDEBUG( dlog, "Starting to consume messages on <" << t_child_it->first << ".#> to queue " << t_child_it->first );
                t_child_it->second->consumer_tag() = t_child_it->second->channel()->BasicConsume( t_child_it->first, "", true, false );
            }
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
            LDEBUG( dlog, "Stopping consuming messages for <" << f_name << "> (consumer " << f_consumer_tag << ")" );
            f_channel->BasicCancel( f_consumer_tag );
            f_consumer_tag.clear();

            for( async_map_t::iterator t_child_it = f_async_children.begin();
                    t_child_it != f_async_children.end();
                    ++t_child_it )
            {
                LDEBUG( dlog, "Stopping consuming messages for <" << t_child_it->first << "> (consumer " << t_child_it->second->consumer_tag() << ")" );
                t_child_it->second->channel()->BasicCancel( t_child_it->second->consumer_tag() );
            }
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

            for( async_map_t::iterator t_child_it = f_async_children.begin();
                    t_child_it != f_async_children.end();
                    ++t_child_it )
            {
                LDEBUG( dlog, "Deleting queue <" << t_child_it->first << ">" );
                t_child_it->second->channel()->DeleteQueue( t_child_it->first, false );
            }
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

        // if the message is already complete, submit it for processing
        if( a_pack.f_chunks_received == a_pack.f_messages.size() )
        {
            t_lock.release(); // process_message() will unlock the mutex before erasing the message pack
            process_message( a_pack, a_message_id );
            return;
        }

        auto t_now = std::chrono::system_clock::now();

        while( a_pack.f_conv.wait_until( t_lock, t_now + std::chrono::milliseconds(f_single_message_wait_ms) ) == std::cv_status::no_timeout )
        {
            // if the message is complete during the waiting period, submit it for processing
            if( a_pack.f_chunks_received == a_pack.f_messages.size() )
            {
                t_lock.release(); // process_message() will unlock the mutex before erasing the message pack
                process_message( a_pack, a_message_id );
                return;
            }
        }

        // once the waiting period is over, submit it whether it's complete or not
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
            incoming_messages().erase( a_message_id );

            f_message_queue.push( t_message );

            return;
        }
        catch( dripline_error& e )
        {
            LERROR( dlog, "Dripline exception caught while handling message: " << e.what() );
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Standard exception caught while sending reply: " << e.what() );
        }

        // TODO: send reply if message was a request?

        return;
    }


    bool service::listen_on_queue()
    {
        LINFO( dlog, "Listening for incoming messages on <" << f_name << ">" );

        while( ! f_canceled.load()  )
        {

            amqp_envelope_ptr t_envelope;
            bool t_channel_valid = core::listen_for_message( t_envelope, f_channel, f_consumer_tag, f_listen_timeout_ms );

            if( f_canceled.load() )
            {
                LDEBUG( dlog, "Service canceled" );
                return true;
            }

            if( ! t_envelope && t_channel_valid )
            {
                // we end up here every time the listen times out with no message received
                continue;
            }

            f_status = status::processing;

            try
            {
                amqp_message_ptr t_message = t_envelope->Message();

                // TODO if the number of chunks expected is 1, pass this directly to be processed

                auto t_parsed_message_id = message::parse_message_id( t_message->MessageId() );
                if( incoming_messages().count( std::get<0>(t_parsed_message_id) ) == 0 )
                {
                    // this path: first chunk for this message
                    // create the new message_pack object
                    incoming_message_pack& t_pack = incoming_messages()[std::get<0>(t_parsed_message_id)];
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
                    incoming_message_pack& t_pack = incoming_messages()[std::get<0>(t_parsed_message_id)];
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
                } // new/current message if/else block
            }
            catch( dripline_error& e )
            {
                LERROR( dlog, "<" << f_name << "> Dripline exception caught while handling message: " << e.what() );
            }
            catch( std::exception& e )
            {
                LERROR( dlog, "<" << f_name << "> Standard exception caught while handling message: " << e.what() );
            }

            if( ! t_channel_valid )
            {
                LERROR( dlog, "Channel is no longer valid for endpoint <" << f_name << ">" );
                return false;
            }

            if( f_canceled.load() )
            {
                LDEBUG( dlog, "Service <" << f_name << "> canceled" );
                return true;
            }

            f_status = status::listening;
        }
        return true;
    }

    void service::submit_message( message_ptr_t a_message )
    {
        try
        {
            std::string t_first_token( a_message->routing_key() );
            t_first_token = t_first_token.substr( 0, t_first_token.find_first_of('.') );
            LDEBUG( dlog, "First token in routing key: <" << t_first_token << ">" );

            if( t_first_token == f_name || t_first_token == f_broadcast_key )
            {
                sort_message( a_message );
            }
            else
            {
                auto t_endpoint_itr = f_sync_children.find( t_first_token );
                if( t_endpoint_itr == f_sync_children.end() )
                {
                    LERROR( dlog, "Did not find child endpoint called <" << t_first_token << ">" );
                    throw dripline_error() << "Did not find child endpoint <" << t_first_token << ">";
                }

                t_endpoint_itr->second->sort_message( a_message );
            }

            // by this point we assume a reply has been sent
            return;
        }
        catch( dripline_error& e )
        {
            LERROR( dlog, "<" << f_name << "> Dripline exception caught while handling message: " << e.what() );
        }
        catch( amqp_exception& e )
        {
            LERROR( dlog, "<" << f_name << "> AMQP exception caught while sending reply: (" << e.reply_code() << ") " << e.reply_text() );
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "<" << f_name << "> AMQP Library Exception caught while sending reply: (" << e.ErrorCode() << ") " << e.what() );
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "<" << f_name << "> Standard exception caught while sending reply: " << e.what() );
        }

        // TODO: each of the above catch sections can generate a reply if the message is a request

        return;
    }

    void service::send_reply( reply_ptr_t a_reply ) const
    {
        LDEBUG( dlog, "Sending reply message to <" << a_reply->routing_key() << ">:\n" <<
                 "    Return code: " << a_reply->get_return_code() << '\n' <<
                 "    Return message: " << a_reply->return_msg() << '\n' <<
                 "    Payload:\n" << a_reply->payload() );

        if( ! send( a_reply ) )
        {
            LWARN( dlog, "Something went wrong while sending the reply" );
        }
        return;
    }

    void service::do_cancellation( int a_code )
    {
        for( async_map_t::iterator t_child_it = f_async_children.begin();
                t_child_it != f_async_children.end();
                ++t_child_it )
        {
            LDEBUG( dlog, "Canceling child endpoint <" << t_child_it->first << ">" );
            t_child_it->second->cancel( a_code );
        }
    }

} /* namespace dripline */
