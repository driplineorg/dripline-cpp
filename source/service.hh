/*
 * service.hh
 *
 *  Created on: Jan 5, 2016
 *      Author: nsoblath
 */

#ifndef DRIPLINE_SERVICE_HH_
#define DRIPLINE_SERVICE_HH_

#include "core.hh"
#include "endpoint.hh"

#include "member_variables.hh"

#include <atomic>
#include <memory>
#include <string>
#include <set>

namespace dripline
{

    class DRIPLINE_API service : public core, public endpoint
    {
        public:
            service( const scarab::param_node& a_config = scarab::param_node(), const std::string& a_queue_name = "",  const std::string& a_broker_address = "", unsigned a_port = 0, const std::string& a_auth_file = "", const bool a_make_connection = true );
            service( const bool a_make_connection, const scarab::param_node& a_config = scarab::param_node() );
            service( const service& ) = delete;
            service( service&& ) = delete;
            virtual ~service();

            service& operator=( const service& ) = delete;
            service& operator=( service&& ) = delete;

        public:
            /// Sends a request message and returns a channel on which to listen for a reply.
            virtual rr_pkg_ptr send( request_ptr_t a_request ) const;

            /// Sends a reply message
            virtual bool send( reply_ptr_t a_reply ) const;

            /// Sends an alert message
            virtual bool send( alert_ptr_t a_alert ) const;

        public:
            /// Creates a channel to the broker and establishes the queue for receiving messages.
            /// If no queue name was given, this does nothing.
            bool start();

            /// Starts listening on the queue for receiving messages.
            /// If no queue was created, this does nothing.
            bool listen();

            /// Stops receiving messages and closes the connection to the broker.
            /// If no queue was created, this does nothing.
            bool stop();

        protected:
            // set the routing key specifier by removing the queue name or broadcast key from the beginning of the routing key
            bool set_routing_key_specifier( message_ptr_t a_message ) const;

        private:
            bool bind_keys( const std::set< std::string >& a_keys );

            bool start_consuming();

            bool stop_consuming();

            bool remove_queue();

        public:
            mv_referrable_const( amqp_channel_ptr, channel );

            mv_referrable_const( std::string, consumer_tag );

            mv_referrable( std::set< std::string >, keys );
            mv_referrable( std::string, broadcast_key );

            mv_accessible( unsigned, listen_timeout_ms );

        protected:
            std::atomic< bool > f_canceled;
    };

} /* namespace dripline */

#endif /* DRIPLINE_SERVICE_HH_ */
