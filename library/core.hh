/*
 * core.hh
 *
 *  Created on: Jul 13, 2015
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_CORE_HH_
#define DRIPLINE_CORE_HH_

#include "dripline_config.hh"
#include "message.hh"

#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

namespace scarab
{
    class authentication;
    class param_node;
}

namespace BloombergLP { namespace rmqa { class Consumer; class RabbitContext; class VHost; class Producer; } }

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
        bsl::shared_ptr< BloombergLP::rmqa::Consumer > f_reply_consumer; ///< rmqcpp consumer on the temporary reply queue (null if no reply expected)
        std::shared_ptr< std::promise< reply_ptr_t > > f_reply_promise;  ///< fulfilled by the reply consumer callback when the reply is assembled
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

        public:
            /* 
               \brief Extracts necessary configuration and authentication information and prepares the DL object to interact with the RabbitMQ broker. Does not initiate connection to the broker.
               @param a_config Dripline configuration object.  Contents can be:
                 - `broker` (string; default: localhost) -- Address of the RabbitMQ broker
                 - `broker_port` (int; default: 5672) -- Port used by the RabbitMQ broker
                 - `requests_exchange` (string; default: requests) -- Name of the exchange used for DL requests
                 - `alerts_exchange` (string; default: alerts) -- Name of the exchange used for DL alerts
                 - `heartbeat_routing_key` (string; default: heartbeat) -- Routing key used for sending heartbeats
                 - `make_connection` (bool; default: true) -- Flag for performing a dry run -- no connection to a broker is made; this parameter overrides the parameter in the constructor and is the preferred flag to use.
                 - `max_payload_size` (int; default: DL_MAX_PAYLOAD_SIZE) -- Maximum size of payloads, in bytes
                 - `max_connection_attempts` (int; default: 10) -- Maximum number of attempts that will be made to connect to the broker
                 - `return_codes` (string or array of nodes; default: not present) -- Optional specification of additional return codes in the form of an array of nodes: `[{name: "<name>", value: <ret code>} <, ...>]`. 
                        If this is a string, it's treated as a file can be interpreted by the param system (e.g. YAML or JSON) using the previously-mentioned format
               @param a_auth Authentication object (type scarab::authentication); authentication specification should be processed, and the authentication data should include:
               @param a_make_connection Flag for whether or not to contact a broker; if true, this object operates in "dry-run" mode
             */
            core( const scarab::param_node& a_config = dripline_config(), const scarab::authentication& a_auth = scarab::authentication(), const bool a_make_connection = true );
            core( const core& a_orig ) = default;
            core( core&& a_orig ) = default;
            virtual ~core() = default;

            core& operator=( const core& a_orig ) = default;
            core& operator=( core&& a_orig ) = default;

        public:
            /// Sends a request message and waits for a reply via an rmqcpp consumer on a temporary queue.
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
            mv_accessible( unsigned, max_connection_attempts );

        protected:
            friend class receiver;

            sent_msg_pkg_ptr do_send( message_ptr_t a_message, const std::string& a_exchange, bool a_expect_reply ) const;

            /// Sets up a temporary reply queue, starts an rmqcpp consumer on it, then sends the message.
            /// Stores the consumer and reply promise in a_pkg.
            void send_withreply( message_ptr_t a_message, const std::string& a_exchange, sent_msg_pkg_ptr a_pkg ) const;

            bool send_noreply( message_ptr_t a_message, const std::string& a_exchange ) const;

            /// Lazily establishes the RabbitMQ connection and creates the requests/alerts producers.
            /// Thread-safe; subsequent calls are no-ops if already connected.
            void open_connection() const;

            mutable bsl::shared_ptr< BloombergLP::rmqa::RabbitContext > f_rabbit_context;
            mutable bsl::shared_ptr< BloombergLP::rmqa::VHost > f_vhost;
            mutable bsl::shared_ptr< BloombergLP::rmqa::Producer > f_requests_producer;
            mutable bsl::shared_ptr< BloombergLP::rmqa::Producer > f_alerts_producer;
            mutable std::shared_ptr< std::mutex > f_connection_mutex;
    };

} /* namespace dripline */

#endif /* DRIPLINE_CORE_HH_ */
