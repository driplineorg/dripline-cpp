/*
 * heartbeat.cc
 *
 *  Created on: Apr 26, 2019
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "heartbeater.hh"

#include "message.hh"
#include "service.hh"

#include "logger.hh"
#include "param_node.hh"

#include <iomanip>

LOGGER( dlog, "heartbeater" );

namespace dripline
{

    heartbeater::heartbeater( service_ptr_t a_service ) :
            cancelable(),
            f_heartbeat_interval_s( 60 ),
            f_check_timeout_ms( 1000 ),
            f_service( a_service ),
            f_heartbeat_thread()
    {}

    heartbeater::heartbeater( heartbeater&& a_orig ) :
            cancelable( std::move(a_orig) ),
            f_heartbeat_interval_s( a_orig.f_heartbeat_interval_s ),
            f_check_timeout_ms( a_orig.f_check_timeout_ms ),
            f_service( std::move(a_orig.f_service) ),
            f_heartbeat_thread( std::move(a_orig.f_heartbeat_thread) )
    {}

    heartbeater::~heartbeater()
    {}

    heartbeater& heartbeater::operator=( heartbeater&& a_orig )
    {
        f_heartbeat_interval_s = std::move(a_orig.f_heartbeat_interval_s);
        f_check_timeout_ms = std::move(a_orig.f_check_timeout_ms);
        f_service = std::move(a_orig.f_service);
        f_heartbeat_thread = std::move(a_orig.f_heartbeat_thread);
        return *this;
    }

    void heartbeater::execute( const std::string& a_name, uuid_t a_id, const std::string& a_routing_key )
    {
        if( ! f_service )
        {
            throw dripline_error() << "Unable to start heartbeater because service pointer is not set";
        }

        if( f_heartbeat_interval_s == 0 )
        {
            LINFO( dlog, "Heartbeat disabled" );
            return;
        }

        scarab::param_ptr_t t_payload_ptr( new scarab::param_node() );
        scarab::param_node& t_payload = t_payload_ptr->as_node();
        t_payload.add( "name", a_name );
        t_payload.add( "id", string_from_uuid(a_id) );

        routing_key t_key;
        t_key.push_back( a_routing_key );
        t_key.push_back( a_name );

        alert_ptr_t t_alert_ptr = msg_alert::create( std::move(t_payload_ptr), t_key.to_string() );

        LINFO( dlog, "Starting heartbeat loop" );

        auto t_next_heartbeat_at = std::chrono::steady_clock().now() + std::chrono::seconds( f_heartbeat_interval_s );
        while( ! f_canceled.load() )
        {
            // wait the interval
            std::this_thread::sleep_for( std::chrono::milliseconds( f_check_timeout_ms ) );

            // send the message
            if( std::chrono::steady_clock().now() >= t_next_heartbeat_at && ! f_canceled.load() )
            {
                LDEBUG( dlog, "Sending heartbeat" );
                // reset message ID so it's different for each heartbeat
                t_alert_ptr->message_id() = string_from_uuid( generate_random_uuid() );

                sent_msg_pkg_ptr t_receive_reply;
                try
                {
                    t_receive_reply = f_service->send( t_alert_ptr );
                }
                catch( message_ptr_t )
                {
                    LWARN( dlog, "Operating in offline mode; message not sent" );
                }
                catch( connection_error& e )
                {
                    LERROR( dlog, "Unable to connect to the broker:\n" << e.what() );
                }
                catch( dripline_error& e )
                {
                    LERROR( dlog, "Dripline error while sending reply:\n" << e.what() );
                }

                if( ! t_receive_reply->f_successful_send )
                {
                    LERROR( dlog, "Failed to send reply:\n" + t_receive_reply->f_send_error_message );
                }
                
                t_next_heartbeat_at = std::chrono::steady_clock().now() + std::chrono::seconds( f_heartbeat_interval_s );
            }
        }

        return;
    }

} /* namespace dripline */


