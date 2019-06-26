/*
 * listener.hh
 *
 *  Created on: Jun 23, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_LISTENER_HH_
#define DRIPLINE_LISTENER_HH_

#include "endpoint.hh"

#include "cancelable.hh"
#include "member_variables.hh"

#include <thread>

namespace dripline
{
    class listener : public virtual scarab::cancelable
    {
        public:
            listener();
            listener( const listener& ) = delete;
            listener( listener&& a_orig );
            virtual ~listener();

            listener& operator=( const listener& ) = delete;
            listener& operator=( listener&& a_orig );

            virtual bool listen_on_queue() = 0;

            mv_referrable( amqp_channel_ptr, channel );

            mv_referrable( std::string, consumer_tag );

            mv_accessible( unsigned, listen_timeout_ms );

            mv_referrable( std::thread, thread );
    };

    class listener_endpoint : public listener
    {
        public:
            listener_endpoint( endpoint_ptr_t a_endpoint_ptr );
            listener_endpoint( const listener_endpoint& ) = delete;
            listener_endpoint( listener_endpoint&& a_orig );
            virtual ~listener_endpoint();

            listener_endpoint& operator=( const listener_endpoint& ) = delete;
            listener_endpoint& operator=( listener_endpoint&& a_orig );

            virtual bool listen_on_queue();

        protected:
            mv_referrable( endpoint_ptr_t,  endpoint );
    };



} /* namespace dripline */

#endif /* DRIPLINE_LISTENER_HH_ */
