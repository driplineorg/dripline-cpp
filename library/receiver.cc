/*
 * receiver.cc
 *
 *  Created on: Feb 18, 2019
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "receiver.hh"

#include "core.hh"
#include "dripline_error.hh"
#include "message.hh"

#include "logger.hh"

LOGGER( dlog, "receiver" );

namespace dripline
{

    receiver::receiver() :
            f_incoming_messages(),
            f_single_message_wait_ms( 1000 )
    {
    }

    receiver::~receiver()
    {
    }

    void receiver::handle_message_chunk( amqp_envelope_ptr a_envelope )
    {
        try
        {
            amqp_message_ptr t_message = a_envelope->Message();
            LDEBUG( dlog, "Received a message chunk <" << t_message->MessageId() );

            // TODO if the number of chunks expected is 1, pass this directly to be processed

            auto t_parsed_message_id = message::parse_message_id( t_message->MessageId() );
            if( incoming_messages().count( std::get<0>(t_parsed_message_id) ) == 0 )
            {
                // this path: first chunk for this message
                LDEBUG( dlog, "This is the first chunk for this message; creating new message pack" );
                // create the new message_pack object
                incoming_message_pack& t_pack = incoming_messages()[std::get<0>(t_parsed_message_id)];
                // set the f_messages vector to the expected size
                t_pack.f_messages.resize( std::get<2>(t_parsed_message_id) );
                // put in place the first message chunk received
                t_pack.f_messages[std::get<1>(t_parsed_message_id)] = t_message;
                t_pack.f_routing_key = a_envelope->RoutingKey();
                t_pack.f_chunks_received = 1;

                // start the thread to wait for message chunks
                t_pack.f_thread = std::thread([this, &t_pack, &t_parsed_message_id](){ wait_for_message(t_pack, std::get<0>(t_parsed_message_id)); });
                t_pack.f_thread.detach();
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
        bool t_temp;
        return wait_for_reply( a_receive_reply, t_temp, a_timeout_ms );
    }

    reply_ptr_t receiver::wait_for_reply( const sent_msg_pkg_ptr a_receive_reply, bool& a_chan_valid, int a_timeout_ms )
    {
        if ( ! a_receive_reply->f_channel )
        {
            return reply_ptr_t();
        }

        LDEBUG( dlog, "Waiting for a reply" );

        // wait for messages until either:
        //   1. the channel is no longer valid (return empty reply pointer; a_chan_valid will be false)
        //   2. listening times out (return empty reply pointer; a_chan_valid will be true)
        //   3. a full dripline message is received (return message)
        //   4. error processing a recieved amqp message (return empty reply pointer)
        while( true )
        {
            amqp_envelope_ptr t_envelope;
            a_chan_valid = core::listen_for_message( t_envelope, a_receive_reply->f_channel, a_receive_reply->f_consumer_tag, a_timeout_ms, false );

            // there was an error listening on the channel; no message received
            if( ! a_chan_valid )
            {
                LDEBUG( dlog, "There was some error while listening on the channel; no message received" );
                return reply_ptr_t();
            }

            // listening timed out
            if( ! t_envelope )
            {
                LDEBUG( dlog, "An empty envelope was returned from listening to a channel; listening may have timed out" );
                return reply_ptr_t();
            }

            try
            {
                amqp_message_ptr t_message = t_envelope->Message();
                LDEBUG( dlog, "Received a message chunk <" << t_message->MessageId() );

                auto t_parsed_message_id = message::parse_message_id( t_message->MessageId() );
                if( f_incoming_messages.count( std::get<0>(t_parsed_message_id) ) == 0 )
                {
                    // this path: first chunk for this message
                    LDEBUG( dlog, "This is the first chunk for this message; creating new message pack" );
                    // create the new message_pack object
                    incoming_message_pack& t_pack = f_incoming_messages[std::get<0>(t_parsed_message_id)];
                    // set the f_messages vector to the expected size
                    t_pack.f_messages.resize( std::get<2>(t_parsed_message_id) );
                    // put in place the first message chunk received
                    t_pack.f_messages[std::get<1>(t_parsed_message_id)] = t_message;
                    t_pack.f_routing_key = t_envelope->RoutingKey();
                    t_pack.f_chunks_received = 1;

                    LWARN( dlog, "chunks received: " << t_pack.f_chunks_received << "   message size: " << t_pack.f_messages.size() );
                    if( t_pack.f_chunks_received == t_pack.f_messages.size() )
                    {
                        return process_received_reply( t_pack, std::get<0>(t_parsed_message_id) );
                    }
                }
                else
                {
                    // this path: have already received chunks from this message
                    LDEBUG( dlog, "This is not the first chunk for this message; adding to message pack" );
                    incoming_message_pack& t_pack = f_incoming_messages[std::get<0>(t_parsed_message_id)];
                    if( t_pack.f_processing.load() )
                    {
                        LWARN( dlog, "Message <" << std::get<0>(t_parsed_message_id) << "> is already being processed\n" <<
                                "Just received chunk " << std::get<1>(t_parsed_message_id) << " of " << std::get<2>(t_parsed_message_id) );
                    }
                    else
                    {
                        if( t_pack.f_messages[std::get<1>(t_parsed_message_id)] )
                        {
                            LWARN( dlog, "Received duplicate message chunk for message <" << std::get<0>(t_parsed_message_id) << ">; chunk " << std::get<1>(t_parsed_message_id) );
                        }
                        else
                        {
                            // add chunk to set of chunks
                            t_pack.f_messages[std::get<1>(t_parsed_message_id)] = t_message;
                            ++t_pack.f_chunks_received;
                            LWARN( dlog, "chunks received: " << t_pack.f_chunks_received << "   message size: " << t_pack.f_messages.size() );
                            if( t_pack.f_chunks_received == t_pack.f_messages.size() )
                            {
                                return process_received_reply( t_pack, std::get<0>(t_parsed_message_id) );
                            }
                        }
                    }
                }

            }
            catch( dripline_error& e )
            {
                LERROR( dlog, "There was a problem processing the message: " << e.what() );
                return reply_ptr_t();
            }
        } // end while( true )
    }

    reply_ptr_t receiver::process_received_reply( incoming_message_pack& a_pack, const std::string& a_message_id )
    {
        a_pack.f_processing.store( true );
        try
        {
            message_ptr_t t_message = message::process_message( a_pack.f_messages, a_pack.f_routing_key );

            f_incoming_messages.erase( a_message_id );

            if( t_message->is_reply() )
            {
                return std::static_pointer_cast< msg_reply >( t_message );
            }
            else
            {
                throw dripline_error() << "Non-reply message received";
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

        return reply_ptr_t();

    }

    concurrent_receiver::concurrent_receiver() :
            scarab::cancelable(),
            receiver(),
            f_message_queue()
    {
    }

    concurrent_receiver::~concurrent_receiver()
    {
    }

    void concurrent_receiver::process_message( message_ptr_t a_message )
    {
        f_message_queue.push( a_message );
        return;
    }

    void concurrent_receiver::execute()
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



} /* namespace dripline */
