/*
 * listener.cc
 *
 *  Created on: Jun 23, 2019
 *      Author: N.S. Oblath
 */

#include "listener.hh"

#include "core.hh"
#include "dripline_error.hh"

#include "logger.hh"

namespace dripline
{
    LOGGER( dlog, "listener" );

    listener::listener() :
            cancelable(),
            f_channel(),
            f_consumer_tag(),
            f_listen_timeout_ms( 0 ),
            f_thread()
    {}

    listener::listener( listener&& a_orig ) :
            cancelable( a_orig ),
            f_channel( std::move(a_orig.f_channel) ),
            f_consumer_tag( std::move(a_orig.f_consumer_tag) ),
            f_listen_timeout_ms( std::move(a_orig.f_listen_timeout_ms) ),
            f_thread( std::move(a_orig.f_thread) )
    {}

    listener::~listener()
    {}

    listener& listener::operator=( listener&& a_orig )
    {
        cancelable::operator=( std::move(a_orig) );
        f_channel = std::move(a_orig.f_channel);
        f_consumer_tag = std::move(a_orig.f_consumer_tag);
        f_consumer_tag = std::move(a_orig.f_listen_timeout_ms);
        f_thread = std::move(a_orig.f_thread);
        return *this;
    }

    listener_endpoint::listener_endpoint( endpoint_ptr_t a_endpoint_ptr ) :
            listener(),
            f_endpoint( a_endpoint_ptr )
    {}

    listener_endpoint::listener_endpoint( listener_endpoint&& a_orig ) :
            listener( std::move(a_orig) ),
            f_endpoint( std::move(a_orig.f_endpoint) )
    {}

    listener_endpoint::~listener_endpoint()
    {}

    listener_endpoint& listener_endpoint::operator=( listener_endpoint&& a_orig )
    {
        listener::operator=( std::move(a_orig) );
        f_endpoint = std::move(a_orig.f_endpoint);
        return *this;
    }

    bool listener_endpoint::listen_on_queue()
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

            try
            {
                message_ptr_t t_message = message::process_envelope( t_envelope );

                f_endpoint->sort_message( t_message );
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

} /* namespace dripline */
