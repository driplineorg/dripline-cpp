/*
 * heartbeat.cc
 *
 *  Created on: Apr 26, 2019
 *      Author: N.S. Oblath
 */

#include "heartbeat.hh"

#include <thread>

namespace dripline
{

    heartbeat::heartbeat( service* a_service ) :
            cancelable(),
            f_service( a_service ),
            f_stop( false )
    {}

    heartbeat::~heartbeat()
    {}

    void heartbeat::execute( const std::string& a_name, uuid_t a_id, unsigned an_interval, const std::string& a_routing_key )
    {
        scarab::param_node t_payload;
        t_payload.add( "name", a_name );
        t_payload.add( "id", string_from_uuid(a_id) );

        while( ! this->is_canceled() && ! f_stop.load() )
        {
            // wait the interval
            std::this_thread::sleep_for( std::chrono::seconds( an_interval ) );

            // send the message
            scarab::param_ptr_t t_payload_copy = t_payload.clone();
            alert_ptr_t t_alert_ptr = msg_alert::create( std::move(t_payload_copy), a_routing_key );

            if( ! f_service->is_canceled() )
            {
                f_service->send( t_alert_ptr );
            }
            else
            {
                cancel();
            }
        }

        return;
    }

} /* namespace dripline */


