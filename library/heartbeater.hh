/*
 * heatbeater.hh
 *
 *  Created on: Apr 26, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_HEARTBEATER_HH_
#define DRIPLINE_HEARTBEATER_HH_

#include "dripline_api.hh"
#include "dripline_fwd.hh"
#include "uuid.hh"

#include "cancelable.hh"
#include "member_variables.hh"

#include <thread>

namespace dripline
{

    /*!
     @class heartbeater
     @author N.S. Oblath

     @brief Adds heartbeat capabilities: repeatedly sends an alert on a particular time interval

     @details
     This class is intended to be used as a mix-in to add heartbeat capabilities to another class 
     by inheriting from it.  See @ref service as an example of its use.

     The heartbeat is an alert sent to a pre-determined routing key, which is given as a parameter to the 
     `execute()` function.  The interval for sending the heartbeats is `f_heartbeat_interval_s`, 
     which is in seconds.  The default interval is 60 s.

     If the heartbeat interval is 0, the sending of heartbeats is disabled.

     The parameters provided upon execution are the name (`a_name`, e.g. the service name), a UUID (`a_id`), and 
     the routing key (`a_routing_key`).

     The payload of each heartbeat message will be:
     ~~~
     {
         name: [a_name]
         id: [a_id]
     }
     ~~~

     The routing key to which the message will be sent is `a_routing_key.a_name`.
    */
    class DRIPLINE_API heartbeater : public virtual scarab::cancelable
    {
        public:
            ///Primary constructor.  A service pointer is required to be able to send messages.
            heartbeater( service_ptr_t a_service = service_ptr_t() );
            heartbeater( const heartbeater& ) = delete;
            heartbeater( heartbeater&& );
            virtual ~heartbeater();

            /*!
             Starts the heartbeat process.  Heartbeat alerts are emitted every `heartbeat_interval_s` seconds.
             If the interval is 0, then heartbeats are disabled.
             @param a_name The name for the heartbeater (e.g. the service)
             @param a_id UUID for the heartbeater
             @param a_routing_key The base of the routing key for heartbeat alerts; will be postpended with \ref a_name
            */
            void execute( const std::string& a_name, uuid_t a_id, const std::string& a_routing_key );

            /// Interval between heartbeat alerts (default: 60 s)
            mv_accessible( unsigned, heartbeat_interval_s );
            /// Timing interval for the internal loop (default: 1000 ms)
            mv_accessible( unsigned, check_timeout_ms );

            mv_referrable( service_ptr_t, service );

            mv_atomic( bool, stop );

        protected:
            std::thread f_heartbeat_thread;

    };

} /* namespace dripline */

#endif /* DRIPLINE_HEARTBEATER_HH_ */
