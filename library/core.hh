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
    class authentication;
    class param_node;
}

namespace dripline
{
    /*!
     @class sent_msg_pkg
     @author N.S. Oblath

     @brief Contains information about sent messages

     @details
     `core::send()` will return a `sent_msg_pkg`.

     The result of act of sending the message is given by `f_successful_send` and `f_send_error_message`.

     Replies can be waited for and retried by passing the `sent_msg_pkg` to `receiver::wait_for_reply()`.
    */
    struct DRIPLINE_API sent_msg_pkg
    {
        std::mutex f_mutex;
        amqp_channel_ptr f_channel;
        std::string f_consumer_tag;
        bool f_successful_send;
        std::string f_send_error_message;
        ~sent_msg_pkg();
    };

    /*!
     @class core
     @author N.S. Oblath

     @brief Basic AMQP interactions, including sending messages and interacting with AMQP channels.

     @details
     The configuration for a `core` object is supplied via the constructor.  The basic required information can be obtained 
     from `dripline_config`.  The configuration values have default parameters, and they can be modified with the config 
     `param_node`, and a few parameters can be specified explicitly as constructor arguments.  The order of precedence for 
     those values is (items higher in the list override those below them):
       * Constructor arguments (other than `a_config`)
       * Config `param_node` object
       * Defaults
    
     If the broker is not specified in either the config object or as a constructor parameter, it will be requested from the 
     authentication file.
    
     A second constructor allows a user to create a `core` object without connecting to a broker.

     The primary user interface is `core::send()`, one of which exists for each type of message (request, reply, and alert).

     `Core` also contains a number of utility functions that wrap the main interactions with AMQP channels.  
     Classes wishing to take advantage of those functions should inherit from `core`.
    */
    class DRIPLINE_API core
    {
        public:
            static bool s_offline;

            enum class post_listen_status
            {
                unknown, ///< Initialized or unknown status
                message_received, ///< A message was received, and the channel is still valid
                timeout, ///< A timeout occurred, and the channel is still valid
                soft_error, ///< An error occurred, but the channel should still be valid
                hard_error ///< An error occurred, and the channel is no longer valid
            };

        public:
            /* 
               \brief Extracts necessary configuration and authentication information and prepares the DL object to interact with the RabbitMQ broker. Does not initiate connection to the broker.
               @param a_config Dripline configuration object.  Contents can be:
                 - `broker` (string; default: localhost) -- Address of the RabbitMQ broker
                 - `broker-port` (int; default: 5672) -- Port used by the RabbitMQ broker
                 - `requests-exchange` (string; default: requests) -- Name of the exchange used for DL requests
                 - `alerts-exchange` (string; default: alerts) -- Name of the exchange used for DL alerts
                 - `heartbeat-routing-key` (string; default: heartbeat) -- Routing key used for sending heartbeats
                 - `max-payload-size` (int; default: DL_MAX_PAYLOAD_SIZE) -- Maximum size of payloads, in bytes
                 - `max-connection-attempts` (int; default: 10) -- Maximum number of attempts that will be made to connect to the broker
                 - `return-codes` (string or array of nodes; default: not present) -- Optional specification of additional return codes in the form of an array of nodes: `[{name: "<name>", value: <ret code>} <, ...>]`. 
                        If this is a string, it's treated as a file can be interpreted by the param system (e.g. YAML or JSON) using the previously-mentioned format
               @param a_auth Authentication object (type scarab::authentication); authentication specification should be processed, and the authentication data should include:
               @param a_make_connection Flag for whether or not to contact a broker; if true, this object operates in "dry-run" mode
             */
            core( const scarab::param_node& a_config, const scarab::authentication& a_auth, const bool a_make_connection = true );
//            core( const bool a_make_connection, const scarab::param_node& a_config = scarab::param_node(), const scarab::authentication& a_auth = scarab::authentication() );
            core( const core& a_orig );
            core( core&& a_orig );
            //core( const scarab::param_node* a_config = nullptr );
            virtual ~core();

            core& operator=( const core& a_orig );
            core& operator=( core&& a_orig );

        public:
            /// Sends a request message and returns a channel on which to listen for a reply.
            /// Default exchange is "requests"
            /// Caller can supply a channel; if one is not supplied, a new channel will be established
            virtual sent_msg_pkg_ptr send( request_ptr_t a_request, amqp_channel_ptr a_channel = amqp_channel_ptr() ) const;

            /// Sends a reply message
            /// Default exchange is "requests"
            /// Caller can supply a channel; if one is not supplied, a new channel will be established
            virtual sent_msg_pkg_ptr send( reply_ptr_t a_reply, amqp_channel_ptr a_channel = amqp_channel_ptr() ) const;

            /// Sends an alert message
            /// Default exchange is "alerts"
            /// Caller can supply a channel; if one is not supplied, a new channel will be established
            virtual sent_msg_pkg_ptr send( alert_ptr_t a_alert, amqp_channel_ptr a_channel = amqp_channel_ptr() ) const;

            mv_referrable( std::string, address );
            mv_accessible( unsigned, port );
            mv_referrable( std::string, username );
            mv_referrable( std::string, password );

            mv_referrable( std::string, requests_exchange );
            mv_referrable( std::string, alerts_exchange );

            mv_referrable( std::string, heartbeat_routing_key );

            mv_accessible( unsigned, max_payload_size );

            mv_accessible( bool, make_connection );
            mv_accessible( unsigned, max_connection_attempts );

        protected:
            friend class receiver;

            sent_msg_pkg_ptr do_send( message_ptr_t a_message, const std::string& a_exchange, bool a_expect_reply, amqp_channel_ptr a_channel = amqp_channel_ptr() ) const;

            amqp_channel_ptr send_withreply( message_ptr_t a_message, std::string& a_reply_consumer_tag, const std::string& a_exchange ) const;

            bool send_noreply( message_ptr_t a_message, const std::string& a_exchange ) const;

            amqp_channel_ptr open_channel() const;

            static bool setup_exchange( amqp_channel_ptr a_channel, const std::string& a_exchange );

            static bool setup_queue( amqp_channel_ptr a_channel, const std::string& a_queue_name );

            static bool bind_key( amqp_channel_ptr a_channel, const std::string& a_exchange, const std::string& a_queue_name, const std::string& a_routing_key );

            static std::string start_consuming( amqp_channel_ptr a_channel, const std::string& a_queue_name );

            static bool stop_consuming( amqp_channel_ptr a_channel, std::string& a_consumer_tag );

            static bool remove_queue( amqp_channel_ptr a_channel, const std::string& a_queue_name );

        public:
            /// listen for a single AMQP message
            static void listen_for_message( amqp_envelope_ptr& a_envelope, post_listen_status& a_status, amqp_channel_ptr a_channel, const std::string& a_consumer_tag, int a_timeout_ms = 0, bool a_do_ack = true );
    };

} /* namespace dripline */

#endif /* DRIPLINE_CORE_HH_ */
