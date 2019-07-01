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

    class DRIPLINE_API heartbeater : public virtual scarab::cancelable
    {
        public:
            heartbeater( service_ptr_t a_service = service_ptr_t() );
            heartbeater( const heartbeater& ) = delete;
            heartbeater( heartbeater&& );
            virtual ~heartbeater();

            void execute( const std::string& a_name, uuid_t a_id, const std::string& a_routing_key );

            mv_accessible( unsigned, heartbeat_interval_s );
            mv_accessible( unsigned, check_timeout_ms );

            mv_referrable( service_ptr_t, service );

            mv_atomic( bool, stop );

        protected:
            std::thread f_heartbeat_thread;

    };

} /* namespace dripline */

#endif /* DRIPLINE_HEARTBEATER_HH_ */
