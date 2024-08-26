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

    listener& listener::operator=( listener&& a_orig )
    {
        cancelable::operator=( std::move(a_orig) );
        f_channel = std::move( a_orig.f_channel );
        f_consumer_tag = std::move( a_orig.f_consumer_tag );
        f_listen_timeout_ms = a_orig.f_listen_timeout_ms;
        f_listener_thread = std::move( a_orig.f_listener_thread );
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
            core::post_listen_status t_post_listen_status = core::post_listen_status::unknown;
            core::listen_for_message( t_envelope, t_post_listen_status, f_channel, f_consumer_tag, f_listen_timeout_ms );

            if( f_canceled.load() )
            {
                LDEBUG( dlog, "Service canceled" );
                return true;
            }

            if( t_post_listen_status == core::post_listen_status::timeout )
            {
                // we end up here every time the listen times out with no message received
                continue;
            }

            if( t_post_listen_status == core::post_listen_status::soft_error )
            {
                LWARN( dlog, "A soft error ocurred while listening for messages for <" << f_endpoint->name() << ">.  The channel is still valid" );
                continue;
            }

            if( t_post_listen_status == core::post_listen_status::hard_error )
            {
                LERROR( dlog, "A hard error ocurred while listening for messages for <" << f_endpoint->name() << ">.  The channel is no longer valid" );
                return false;
            }

            if( t_post_listen_status == core::post_listen_status::unknown )
            {
                LERROR( dlog, "An unknown status occurred while listening for messages for <" << f_endpoint->name() << ">" );
                return false;
            }

            // remaining status is core::post_listen_status::message_received

            handle_message_chunk( t_envelope );

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
            return;
        }
        catch( dripline_error& e )
        {
            LERROR( dlog, "<" << f_endpoint->name() << ">: Dripline exception caught while handling message: " << e.what() );
            throw;
        }
        catch( amqp_exception& e )
        {
            LERROR( dlog, "<" << f_endpoint->name() << ">: AMQP exception caught while handling message: (" << e.reply_code() << ") " << e.reply_text() );
            throw;
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "<" << f_endpoint->name() << ">: AMQP Library Exception caught while handling message: (" << e.ErrorCode() << ") " << e.what() );
            throw;
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "<" << f_endpoint->name() << ">: Standard exception caught while handling message: " << e.what() );
            throw;
        }

        return;
    }

} /* namespace dripline */
