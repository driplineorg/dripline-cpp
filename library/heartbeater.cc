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

#include "param_node.hh"

namespace dripline
{

    heartbeater::heartbeater( service_ptr_t a_service ) :
            cancelable(),
            f_heartbeat_interval_s( 60 ),
            f_service( a_service ),
            f_heartbeat_thread()
    {}

    heartbeater::heartbeater( heartbeater&& a_orig ) :
            cancelable( std::move(a_orig) ),
            f_heartbeat_interval_s( a_orig.f_heartbeat_interval_s ),
            f_service( std::move(a_orig.f_service) ),
            f_heartbeat_thread( std::move(a_orig.f_heartbeat_thread) )
    {}

    heartbeater::~heartbeater()
    {}

    void heartbeater::execute( const std::string& a_name, uuid_t a_id, unsigned an_interval, const std::string& a_routing_key )
    {
        if( ! f_service )
        {
            throw dripline_error() << "Unable to start heartbeater because service pointer is not set";
        }

        scarab::param_node t_payload;
        t_payload.add( "name", a_name );
        t_payload.add( "id", string_from_uuid(a_id) );

        while( ! f_canceled.load() )
        {
            // wait the interval
            std::this_thread::sleep_for( std::chrono::seconds( an_interval ) );

            // send the message
            scarab::param_ptr_t t_payload_copy = t_payload.clone();
            alert_ptr_t t_alert_ptr = msg_alert::create( std::move(t_payload_copy), a_routing_key );

            if( ! f_canceled.load() )
            {
                f_service->send( t_alert_ptr );
            }
        }

        return;
    }

} /* namespace dripline */


