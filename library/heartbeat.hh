/*
 * heatbeat.hh
 *
 *  Created on: Apr 26, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_HEARTBEATER_HH_
#define DRIPLINE_HEARTBEATER_HH_

#include "service.hh"

namespace dripline
{

    class DRIPLINE_API heartbeat : public scarab::cancelable
    {
        public:
            heartbeat( service* a_service );
            heartbeat( const heartbeat& ) = delete;
            heartbeat( heartbeat&& ) = delete;
            virtual ~heartbeat();

            void execute( const std::string& a_name, uuid_t a_id, unsigned an_interval, const std::string& a_routing_key );

            mv_assignable( service, service );
            mv_atomic( bool, stop );
    };

} /* namespace dripline */

#endif /* DRIPLINE_HEARTBEATER_HH_ */
