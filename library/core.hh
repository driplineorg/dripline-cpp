/*
 * core.hh
 *
 *  Created on: Jul 13, 2015
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_CORE_HH_
#define DRIPLINE_CORE_HH_

#include "message.hh"

#include <map>
#include <mutex>
#include <thread>

namespace scarab
{
    class param_node;
}

namespace dripline
{
    struct DRIPLINE_API sent_msg_pkg
    {
        std::mutex f_mutex;
        amqp_channel_ptr f_channel;
        std::string f_consumer_tag;
        bool f_successful_send;
        std::string f_send_error_message;
        ~sent_msg_pkg();
    };

    class DRIPLINE_API core
    {
        public:
            static bool s_offline;

        public:
            /// Parameters specified in a_config will override the default values.
            /// Parameters specified as individual parameters will override a_config.
            /// If the broker address is not specified, it will be requested from the authentication file.
            core( const scarab::param_node& a_config = scarab::param_node(), const std::string& a_broker_address = "", unsigned a_port = 0, const std::string& a_auth_file = "", const bool a_make_connection = true );
            core( const bool a_make_connection, const scarab::param_node& a_config = scarab::param_node() );
            core( const core& a_orig );
            core( core&& a_orig );
            //core( const scarab::param_node* a_config = nullptr );
            virtual ~core();

            core& operator=( const core& a_orig );
            core& operator=( core&& a_orig );

        public:
            /// Sends a request message and returns a channel on which to listen for a reply.
            /// Default exchange is "requests"
            virtual sent_msg_pkg_ptr send( request_ptr_t a_request ) const;

            /// Sends a reply message
            /// Default exchange is "requests"
            virtual sent_msg_pkg_ptr send( reply_ptr_t a_reply ) const;

            /// Sends an alert message
            /// Default exchange is "alerts"
            virtual sent_msg_pkg_ptr send( alert_ptr_t a_alert ) const;

            mv_referrable( std::string, address );
            mv_accessible( unsigned, port );
            mv_referrable( std::string, username );
            mv_referrable( std::string, password );

            mv_referrable( std::string, requests_exchange );
            mv_referrable( std::string, alerts_exchange );

            mv_referrable( std::string, heartbeat_routing_key );

            mv_accessible( unsigned, max_payload_size );

            mv_accessible( bool, make_connection );

        protected:
            friend class receiver;

            sent_msg_pkg_ptr do_send( message_ptr_t a_message, const std::string& a_exchange, bool a_expect_reply ) const;

            amqp_channel_ptr send_withreply( message_ptr_t a_message, std::string& a_reply_consumer_tag, const std::string& a_exchange ) const;

            bool send_noreply( message_ptr_t a_message, const std::string& a_exchange ) const;

            amqp_channel_ptr open_channel() const;

            static bool setup_exchange( amqp_channel_ptr a_channel, const std::string& a_exchange );

            static bool setup_queue( amqp_channel_ptr a_channel, const std::string& a_queue_name );

        public:
            /// return: if false, channel is no longer useable; if true, may be reused
            static bool listen_for_message( amqp_envelope_ptr& a_envelope, amqp_channel_ptr a_channel, const std::string& a_consumer_tag, int a_timeout_ms = 0, bool a_do_ack = true );
    };

} /* namespace dripline */

#endif /* DRIPLINE_CORE_HH_ */
