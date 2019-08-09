/*
 * monitor.hh
 *
 *  Created on: Jul 1, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_MONITOR_HH_
#define DRIPLINE_MONITOR_HH_

#include "core.hh"
#include "listener.hh"
#include "receiver.hh"

namespace DRIPLINE_API dripline
{

    class monitor :
            public core,
            public listener_receiver
    {
        protected:
            enum class status
            {
                nothing = 0,
                channel_created = 10,
                exchange_declared = 20,
                queue_declared = 30,
                queue_bound = 40,
                consuming = 50,
                listening = 60
            };

        public:
            monitor( const scarab::param_node& a_config = scarab::param_node() );
            monitor( const monitor& ) = delete;
            monitor( monitor&& a_orig );
            virtual ~monitor();

            monitor& operator=( const monitor& ) = delete;
            monitor& operator=( monitor&& a_orig );

            mv_accessible( status, status );

            mv_referrable( std::string, name ); // automatically set to monitor_[uuid]

            mv_accessible( bool, json_print );
            mv_accessible( bool, pretty_print );

            typedef std::vector< std::string > keys_t;
            mv_referrable( keys_t, requests_keys );
            mv_referrable( keys_t, alerts_keys );

        public:
            bool start();

            bool listen();

            bool stop();

        protected:
            bool bind_keys();

        public:
            virtual bool listen_on_queue();

            virtual void submit_message( message_ptr_t a_message );
    };

} /* namespace dripline */

#endif /* DRIPLINE_MONITOR_HH_ */
