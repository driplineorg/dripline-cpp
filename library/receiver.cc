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

#include <future>

LOGGER( dlog, "receiver" );

namespace dripline
{
    incoming_message_pack::incoming_message_pack() :
            f_messages(),
            f_chunks_received(),
            f_routing_key(),
            f_creation_time(),
            f_mutex()
    {}

    incoming_message_pack::incoming_message_pack( incoming_message_pack&& a_orig ) :
            f_messages( std::move(a_orig.f_messages) ),
            f_chunks_received( a_orig.f_chunks_received ),
            f_routing_key( std::move(a_orig.f_routing_key) ),
            f_creation_time( a_orig.f_creation_time ),
            f_mutex()
    {
        a_orig.f_chunks_received = 0;
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
        return *this;
    }

    void receiver::handle_message_chunk( amqp_envelope_ptr a_envelope )
    {
        try
        {
            amqp_message_ptr t_message = bsl::make_shared< BloombergLP::rmqt::Message >( a_envelope->message() );
            LDEBUG( dlog, "Received a message chunk <" << std::string( t_message->messageId() ) << ">" );

            auto t_parsed = message::parse_message_id( std::string( t_message->messageId() ) );
            const std::string t_message_id( std::get<0>(t_parsed) );
            const unsigned t_chunk_idx = std::get<1>(t_parsed);
            const unsigned t_num_chunks = std::get<2>(t_parsed);

            amqp_split_message_ptrs t_complete_messages;
            std::string t_complete_routing_key;
            bool t_is_complete = false;

            {
                std::lock_guard< std::mutex > t_map_lock( f_incoming_messages_mutex );

                // Lazy stale sweep: evict entries older than f_single_message_wait_ms
                auto t_now = std::chrono::steady_clock::now();
                for( auto it = f_incoming_messages.begin(); it != f_incoming_messages.end(); )
                {
                    auto t_age_ms = std::chrono::duration_cast< std::chrono::milliseconds >( t_now - it->second.f_creation_time ).count();
                    if( static_cast< unsigned >( t_age_ms ) > f_single_message_wait_ms )
                    {
                        LWARN( dlog, "Evicting stale incomplete message <" << it->first << ">" );
                        it = f_incoming_messages.erase( it );
                    }
                    else
                    {
                        ++it;
                    }
                }

                // Find or insert the entry for this message
                bool t_is_new = ( f_incoming_messages.find( t_message_id ) == f_incoming_messages.end() );
                incoming_message_pack& t_pack = f_incoming_messages[ t_message_id ];
                if( t_is_new )
                {
                    LDEBUG( dlog, "First chunk for message <" << t_message_id << ">; creating new message pack" );
                    t_pack.f_messages.resize( t_num_chunks );
                    t_pack.f_routing_key = std::string( a_envelope->envelope().routingKey() );
                    t_pack.f_creation_time = t_now;
                    t_pack.f_chunks_received = 0;
                }

                // Store the chunk under per-entry mutex (lock order: map → entry)
                std::unique_lock< std::mutex > t_entry_lock( t_pack.f_mutex );
                if( t_pack.f_messages[ t_chunk_idx ] )
                {
                    LWARN( dlog, "Received duplicate chunk " << t_chunk_idx << " for message <" << t_message_id << ">; ignoring" );
                }
                else
                {
                    t_pack.f_messages[ t_chunk_idx ] = t_message;
                    ++t_pack.f_chunks_received;
                    LDEBUG( dlog, "Stored chunk " << t_chunk_idx << " for message <" << t_message_id << ">; "
                            << t_pack.f_chunks_received << " / " << t_pack.f_messages.size() << " received" );
                }

                if( t_pack.f_chunks_received == t_pack.f_messages.size() )
                {
                    // Message complete: extract data, release entry lock, then erase from map
                    t_complete_messages = std::move( t_pack.f_messages );
                    t_complete_routing_key = std::move( t_pack.f_routing_key );
                    t_entry_lock.unlock();
                    f_incoming_messages.erase( t_message_id );
                    t_is_complete = true;
                    LDEBUG( dlog, "Message <" << t_message_id << "> complete; processing" );
                }
            } // map lock (and entry lock if not yet released) released here

            if( t_is_complete )
            {
                process_message_pack( t_complete_messages, t_complete_routing_key );
            }
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

    void receiver::process_message_pack( amqp_split_message_ptrs& a_messages, const std::string& a_routing_key )
    {
        try
        {
            message_ptr_t t_message = message::process_message( a_messages, a_routing_key );

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
            f_consumer()
    {}

    concurrent_receiver::concurrent_receiver( concurrent_receiver&& a_orig ) :
            receiver( std::move(a_orig) ),
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
        this->submit_message( a_message );
        return;
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
