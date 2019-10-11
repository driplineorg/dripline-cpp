/*
 * listener.cc
 *
 *  Created on: Jun 23, 2019
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "listener.hh"

#include "endpoint.hh"

#include "core.hh"
#include "dripline_exceptions.hh"

#include "logger.hh"

namespace dripline
{
    LOGGER( dlog, "listener" );

    listener::listener() :
            cancelable(),
            f_channel(),
            f_consumer_tag(),
            f_listen_timeout_ms( 1000 ),
            f_listener_thread()
    {}

    listener::listener( listener&& a_orig ) :
            cancelable( std::move(a_orig) ),
            f_channel( std::move(a_orig.f_channel) ),
            f_consumer_tag( std::move(a_orig.f_consumer_tag) ),
            f_listen_timeout_ms( std::move(a_orig.f_listen_timeout_ms) ),
            f_listener_thread( std::move(a_orig.f_listener_thread) )
    {}

    listener::~listener()
    {}

    listener& listener::operator=( listener&& a_orig )
    {
        cancelable::operator=( std::move(a_orig) );
        f_channel = std::move(a_orig.f_channel);
        f_consumer_tag = std::move(a_orig.f_consumer_tag);
        f_consumer_tag = std::move(a_orig.f_listen_timeout_ms);
        f_listener_thread = std::move(a_orig.f_listener_thread);
        return *this;
    }

    endpoint_listener_receiver::endpoint_listener_receiver( endpoint_ptr_t a_endpoint_ptr ) :
            scarab::cancelable(),
            listener_receiver(),
            f_endpoint( a_endpoint_ptr )
    {}

    endpoint_listener_receiver::endpoint_listener_receiver( endpoint_listener_receiver&& a_orig ) :
            scarab::cancelable( std::move(a_orig) ),
            listener_receiver( std::move(a_orig) ),
            f_endpoint( std::move(a_orig.f_endpoint) )
    {}

    endpoint_listener_receiver::~endpoint_listener_receiver()
    {}

    endpoint_listener_receiver& endpoint_listener_receiver::operator=( endpoint_listener_receiver&& a_orig )
    {
        listener_receiver::operator=( std::move(a_orig) );
        f_endpoint = std::move(a_orig.f_endpoint);
        return *this;
    }

    bool endpoint_listener_receiver::listen_on_queue()
    {
        LINFO( dlog, "Listening for incoming messages on <" << f_endpoint->name() << ">" );

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

            handle_message_chunk( t_envelope );

            if( ! t_channel_valid )
            {
                LERROR( dlog, "Channel is no longer valid for <" << f_endpoint->name() << ">" );
                return false;
            }

            if( f_canceled.load() )
            {
                LDEBUG( dlog, "Listener <" << f_endpoint->name() << "> canceled" );
                return true;
            }
        }
        return true;
    }

    void endpoint_listener_receiver::submit_message( message_ptr_t a_message )
    {
        try
        {
            f_endpoint->sort_message( a_message );

            // by this point we assume a reply has been sent
            return;
        }
        catch( dripline_error& e )
        {
            LERROR( dlog, "<" << f_endpoint->name() << ">: Dripline exception caught while handling message: " << e.what() );
        }
        catch( amqp_exception& e )
        {
            LERROR( dlog, "<" << f_endpoint->name() << ">: AMQP exception caught while sending reply: (" << e.reply_code() << ") " << e.reply_text() );
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "<" << f_endpoint->name() << ">: AMQP Library Exception caught while sending reply: (" << e.ErrorCode() << ") " << e.what() );
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "<" << f_endpoint->name() << ">: Standard exception caught while sending reply: " << e.what() );
        }

        // TODO: each of the above catch sections can generate a reply if the message is a request

        return;
    }

} /* namespace dripline */
