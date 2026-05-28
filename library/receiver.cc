/*
 * receiver.cc
 *
 *  Created on: Feb 18, 2019
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "receiver.hh"

#include "dripline_exceptions.hh"
#include "message.hh"

#include "rmqa_vhost.h"
#include "rmqp_messageguard.h"

#include "logger.hh"
#include "signal_handler.hh"

#include <future>

LOGGER( dlog, "receiver" );

namespace dripline
{
    incoming_message_pack::incoming_message_pack() :
            f_messages(),
            f_chunks_received(),
            f_routing_key(),
            f_thread(),
            f_mutex(),
            f_conv(),
            f_processing( false )
    {}

    incoming_message_pack::incoming_message_pack( incoming_message_pack&& a_orig ) :
            f_messages( std::move(a_orig.f_messages) ),
            f_chunks_received( a_orig.f_chunks_received ),
            f_routing_key( std::move(a_orig.f_routing_key) ),
            f_thread( std::move(a_orig.f_thread) ),
            f_mutex(),
            f_conv(),
            f_processing( a_orig.f_processing.load() )
    {
        a_orig.f_chunks_received = 0;
        a_orig.f_processing.store( false );
    }


    receiver::receiver() :
            scarab::cancelable(),
            f_incoming_messages(),
            f_single_message_wait_ms( 1000 )
    {}

    receiver& receiver::operator=( receiver&& a_orig )
    {
        cancelable::operator=( std::move(a_orig) );
        f_incoming_messages = std::move(a_orig.f_incoming_messages);
        f_single_message_wait_ms = a_orig.f_single_message_wait_ms;
        f_reply_listen_timeout_ms = a_orig.f_reply_listen_timeout_ms;
        return *this;
    }

    void receiver::handle_message_chunk( amqp_envelope_ptr a_envelope )
    {
        try
        {
            amqp_message_ptr t_message = bsl::make_shared< BloombergLP::rmqt::Message >( a_envelope->message() );
            LDEBUG( dlog, "Received a message chunk <" << std::string( t_message->messageId() ) );

            auto t_parsed_message_id = message::parse_message_id( std::string( t_message->messageId() ) );
            std::string t_message_id( std::get<0>(t_parsed_message_id) );
            if( incoming_messages().count( t_message_id ) == 0 )
            {
                // this path: first chunk for this message
                LDEBUG( dlog, "This is the first chunk for this message; creating new message pack" );
                // create the new message_pack object
                incoming_message_pack& t_pack = incoming_messages()[t_message_id];
                // set the f_messages vector to the expected size
                t_pack.f_messages.resize( std::get<2>(t_parsed_message_id) );
                // put in place the first message chunk received
                t_pack.f_messages[std::get<1>(t_parsed_message_id)] = t_message;
                t_pack.f_routing_key = std::string( a_envelope->envelope().routingKey() );
                t_pack.f_chunks_received = 1;

                if( t_pack.f_messages.size() == 1 )
                {
                    // if we only expect one chunk, we can bypass creating a separate thread, etc
                    LDEBUG( dlog, "Single-chunk message being sent directly to processing" );
                    process_message_pack( t_pack, t_message_id );
                }
                else
                {
                    // start the thread to wait for message chunks
                    t_pack.f_thread = std::thread([this, &t_pack, &t_parsed_message_id](){ wait_for_message(t_pack, std::get<0>(t_parsed_message_id)); });
                    t_pack.f_thread.detach();
                }
            }
            else
            {
                // this path: have already received chunks from this message
                LDEBUG( dlog, "This is not the first chunk for this message; adding to message pack" );
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
            LERROR( dlog, "Dripline exception caught while handling message chunk: " << e.what() );
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Standard exception caught while handling message chunk: " << e.what() );
        }

        return;
    }

    void receiver::wait_for_message( incoming_message_pack& a_pack, const std::string& a_message_id )
    {
        std::unique_lock< std::mutex > t_lock( a_pack.f_mutex );

        LDEBUG( dlog, "Waiting for message; chunks received: " << a_pack.f_chunks_received << "  chunks expected: " << a_pack.f_messages.size() );

        // if the message is already complete, submit it for processing
        if( a_pack.f_chunks_received == a_pack.f_messages.size() )
        {
            t_lock.release(); // process_message() will unlock the mutex before erasing the message pack
            process_message_pack( a_pack, a_message_id );
            return;
        }

        auto t_now = std::chrono::system_clock::now();
        while( a_pack.f_conv.wait_until( t_lock, t_now + std::chrono::milliseconds(f_single_message_wait_ms) ) == std::cv_status::no_timeout )
        {
            // if the message is complete during the waiting period, submit it for processing
            if( a_pack.f_chunks_received == a_pack.f_messages.size() )
            {
                t_lock.release(); // process_message() will unlock the mutex before erasing the message pack
                process_message_pack( a_pack, a_message_id );
                return;
            }
        }

        // once the waiting period is over, submit it whether it's complete or not
        t_lock.release(); // process_message() will unlock the mutex before erasing the message pack
        LWARN( dlog, "Timed out; message may be incomplete" );
        process_message_pack( a_pack, a_message_id );

        return;
    }

    void receiver::process_message_pack( incoming_message_pack& a_pack, const std::string& a_message_id )
    {
        a_pack.f_processing.store( true );
        try
        {
            message_ptr_t t_message = message::process_message( a_pack.f_messages, a_pack.f_routing_key );

            a_pack.f_mutex.unlock();
            incoming_messages().erase( a_message_id );

            // if the message is not valid at this point, continue processing it, and we'll deal with it in the endpoint class

            this->process_message( t_message );

            return;
        }
        catch( dripline_error& e )
        {
            LERROR( dlog, "Dripline exception caught while processing message pack: " << e.what() );
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Standard exception caught while processing message pack: " << e.what() );
        }

        return;
    }

    void receiver::process_message( message_ptr_t )
    {
        throw dripline_error() << "Process_message function has not been implemented";
    }

    reply_ptr_t receiver::wait_for_reply( const sent_msg_pkg_ptr a_receive_reply, int a_timeout_ms )
    {
        if( ! a_receive_reply->f_reply_consumer )
        {
            return reply_ptr_t();
        }

        if( a_timeout_ms != 0 )
        {
            LDEBUG( dlog, "Waiting for a reply (timeout: " << a_timeout_ms << " ms)" );
        }
        else
        {
            LDEBUG( dlog, "Waiting for a reply (no timeout)" );
        }

        auto t_future = a_receive_reply->f_reply_promise->get_future();
        auto t_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds( a_timeout_ms );

        while( ! is_canceled() )
        {
            auto t_status = t_future.wait_for( std::chrono::milliseconds( 100 ) );
            if( t_status == std::future_status::ready )
            {
                return t_future.get();
            }
            if( a_timeout_ms > 0 && std::chrono::steady_clock::now() >= t_deadline )
            {
                LWARN( dlog, "Timed out waiting for reply" );
                return reply_ptr_t();
            }
        }

        LDEBUG( dlog, "Receiver canceled while waiting for reply" );
        return reply_ptr_t();
    }

    concurrent_receiver::concurrent_receiver() :
            receiver(),
            f_message_queue(),
            f_consumer()
    {}

    concurrent_receiver::concurrent_receiver( concurrent_receiver&& a_orig ) :
            receiver( std::move(a_orig) ),
            f_message_queue(),
            f_consumer( std::move(a_orig.f_consumer) )
    {}

    concurrent_receiver::~concurrent_receiver()
    {}

    concurrent_receiver& concurrent_receiver::operator=( concurrent_receiver&& a_orig )
    {
        receiver::operator=( std::move(a_orig) );
        f_consumer = std::move(a_orig.f_consumer);
        return *this;
    }

    void concurrent_receiver::process_message( message_ptr_t a_message )
    {
        f_message_queue.push( a_message );
        return;
    }

    void concurrent_receiver::execute()
    {
        try
        {
            while( ! is_canceled() )
            {
                message_ptr_t t_message;
                if( f_message_queue.timed_wait_and_pop( t_message ) )
                {
                    this->submit_message( t_message );
                }
            }
        }
        catch( const std::exception& e )
        {
            // shutdown gracefully on an exception
            LERROR( dlog, "Exception caught; shutting down.\n" << "\t" << e.what() );
            scarab::signal_handler::cancel_all( RETURN_ERROR );
        }
    }

    void concurrent_receiver::start_listening( bsl::shared_ptr< BloombergLP::rmqa::VHost > a_vhost,
                                               const BloombergLP::rmqa::Topology& a_topology,
                                               const BloombergLP::rmqt::QueueHandle& a_queue_handle,
                                               const std::string& a_label )
    {
        using namespace BloombergLP;
        auto t_result = a_vhost->createConsumer(
            a_topology, a_queue_handle,
            [this]( rmqp::MessageGuard& guard ) {
                amqp_envelope_ptr t_envelope = guard.transferOwnership();
                t_envelope->ack();
                handle_message_chunk( std::move(t_envelope) );
            },
            a_label,
            1 );
        if( ! t_result )
        {
            throw connection_error() << "Unable to create consumer: " << t_result.error();
        }
        f_consumer = t_result.value();
    }

    void concurrent_receiver::stop_listening()
    {
        if( f_consumer )
        {
            auto t_result = f_consumer->cancelAndDrain();
            if( ! t_result )
            {
                LWARN( dlog, "Error canceling consumer: " << t_result.error() );
            }
            f_consumer.reset();
        }
    }



} /* namespace dripline */
