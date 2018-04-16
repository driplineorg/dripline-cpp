/*
 * mt_amqp.hh
 *
 *  Created on: Jul 13, 2015
 *      Author: nsoblath
 */

#ifndef DRIPLINE_CORE_HH_
#define DRIPLINE_CORE_HH_

#include "dripline_api.hh"
#include "message.hh"


namespace scarab
{
    class param_node;
}

namespace dripline
{
    struct receive_reply_pkg
    {
        amqp_channel_ptr f_channel;
        std::string f_consumer_tag;
        bool f_successful_send;
        ~receive_reply_pkg();
    };
    typedef std::shared_ptr< receive_reply_pkg > rr_pkg_ptr;

    class DRIPLINE_API core
    {
        public:
            /// Parameters specified in a_config will override the default values.
            /// Parameters specified as individual parameters will override a_config.
            /// If the broker address is not specified, it will be requested from the authentication file.
            core( const scarab::param_node* a_config = nullptr, const std::string& a_broker_address = "", unsigned a_port = 0, const std::string& a_auth_file = "", const bool a_make_connection = true );
            core( const bool a_make_connection, const scarab::param_node* a_config = nullptr );
            core( const core& a_orig );
            core( core&& a_orig );
            //core( const scarab::param_node* a_config = nullptr );
            virtual ~core();

            core& operator=( const core& a_orig );
            core& operator=( core&& a_orig );

        public:
            /// Sends a request message and returns a channel on which to listen for a reply.
            /// Default exchange is "requests"
            virtual rr_pkg_ptr send( request_ptr_t a_request ) const;

            /// Sends a reply message
            /// Default exchange is "requests"
            virtual bool send( reply_ptr_t a_reply ) const;

            /// Sends an alert message
            /// Default exchange is "alerts"
            virtual bool send( alert_ptr_t a_alert ) const;

            /// Wait for a reply message
            /// If the timeout is <= 0 ms, there will be no timeout
            /// This function can be called multiple times to receive multiple replies
            /// The optional bool argument a_chan_valid will return whether or not the channel is still valid for use
            static reply_ptr_t wait_for_reply( const rr_pkg_ptr a_receive_reply, int a_timeout_ms = 0 );
            static reply_ptr_t wait_for_reply( const rr_pkg_ptr a_receive_reply, bool& a_chan_valid, int a_timeout_ms = 0 );

            mv_referrable( std::string, address );
            mv_accessible( unsigned, port );
            mv_referrable( std::string, username );
            mv_referrable( std::string, password );

            mv_referrable( std::string, requests_exchange );
            mv_referrable( std::string, alerts_exchange );

            mv_accessible( bool, make_connection );

        protected:
            amqp_channel_ptr send_withreply( message_ptr_t a_message, std::string& a_reply_consumer_tag, const std::string& a_exchange ) const;

            bool send_noreply( message_ptr_t a_message, const std::string& a_exchange ) const;

            amqp_channel_ptr open_channel() const;

            static bool setup_exchange( amqp_channel_ptr a_channel, const std::string& a_exchange );

            static bool setup_queue( amqp_channel_ptr a_channel, const std::string& a_queue_name );

            /// return: if false, channel is no longer useable; if true, may be reused
            static bool listen_for_message( amqp_envelope_ptr& a_envelope, amqp_channel_ptr a_channel, const std::string& a_consumer_tag, int a_timeout_ms = 0 );

    };

} /* namespace dripline */

#endif /* DRIPLINE_CORE_HH_ */
