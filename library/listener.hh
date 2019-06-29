/*
 * listener.hh
 *
 *  Created on: Jun 23, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_LISTENER_HH_
#define DRIPLINE_LISTENER_HH_

#include "dripline_fwd.hh"

#include "receiver.hh"

#include "cancelable.hh"
#include "member_variables.hh"

#include <thread>

namespace dripline
{
    class DRIPLINE_API listener : public virtual scarab::cancelable
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

            mv_referrable( std::thread, listener_thread );
            mv_referrable( std::thread, receiver_thread );
    };

    class DRIPLINE_API endpoint_listener_receiver : public listener, public concurrent_receiver
    {
        public:
            endpoint_listener_receiver( endpoint_ptr_t a_endpoint_ptr );
            endpoint_listener_receiver( const endpoint_listener_receiver& ) = delete;
            endpoint_listener_receiver( endpoint_listener_receiver&& a_orig );
            virtual ~endpoint_listener_receiver();

            endpoint_listener_receiver& operator=( const endpoint_listener_receiver& ) = delete;
            endpoint_listener_receiver& operator=( endpoint_listener_receiver&& a_orig );

            virtual bool listen_on_queue();

        protected:
            virtual void submit_message( message_ptr_t a_message );

            mv_referrable( endpoint_ptr_t,  endpoint );
    };



} /* namespace dripline */

#endif /* DRIPLINE_LISTENER_HH_ */
