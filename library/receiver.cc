/*
 * receiver.cc
 *
 *  Created on: Feb 18, 2019
 *      Author: N.S. Oblath
 */

#include "receiver.hh"

#include "core.hh"
#include "dripline_error.hh"

#include "logger.hh"

LOGGER( dlog, "receiver" );

namespace dripline
{

    receiver::receiver() :
            f_incoming_messages()
    {
    }

    receiver::~receiver()
    {
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
            //throw dripline_error() << "cannot wait for reply with make_connection is false";
            return reply_ptr_t();
        }

        LDEBUG( dlog, "Waiting for a reply" );

        // TODO: need to change the flow here
        //         return to listening until there's a timeout or a message is complete
        //         if message is complete, return processed message
        //         if no message, and timeout instead, return null pointer

        // TODO: Also, consider whether these wait-for-reply functions and the capability to
        //       receive broken-up messages should go in a separate class, message_receiver or something

        amqp_envelope_ptr t_envelope;
        a_chan_valid = core::listen_for_message( t_envelope, a_receive_reply->f_channel, a_receive_reply->f_consumer_tag, a_timeout_ms );
        a_receive_reply->f_channel.reset();

        try
        {
            amqp_message_ptr t_message = t_envelope->Message();

            auto t_parsed_message_id = message::parse_message_id( t_message->MessageId() );
            if( f_incoming_messages.count( std::get<0>(t_parsed_message_id) ) == 0 )
            {
                // this path: first chunk for this message
                // create the new message_pack object
                incoming_message_pack& t_pack = f_incoming_messages[std::get<0>(t_parsed_message_id)];
                // set the f_messages vector to the expected size
                t_pack.f_messages.resize( std::get<2>(t_parsed_message_id) );
                // put in place the first message chunk received
                t_pack.f_messages[std::get<1>(t_parsed_message_id)] = t_message;
                t_pack.f_routing_key = t_envelope->RoutingKey();

                if( t_pack.f_chunks_received == t_pack.f_messages.size() )
                {
                    return process_received_reply( t_pack, std::get<0>(t_parsed_message_id) );
                }
            }
            else
            {
                // this path: have already received chunks from this message
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

} /* namespace dripline */
