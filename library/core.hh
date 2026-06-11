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

     @brief Contains information about a sent message and, for requests, the mechanism to wait for a reply.

     @details
     `core::send()` returns a `sent_msg_pkg_ptr` after sending a message.

     The result of the send is given by `f_successful_send` (true if the message was accepted for
     delivery) and `f_send_error_message` (non-empty only on failure).

     For **request** messages (`core::send( request_ptr_t )`):
     - `f_reply_consumer` holds the rmqcpp consumer on a temporary auto-delete reply queue.
     - `f_reply_promise` is fulfilled by the consumer's callback when the complete reply arrives.
     - Call `receiver::wait_for_reply()` with this package to block until the reply is assembled.

     For **reply** and **alert** messages there is no reply expected:
     - `f_reply_consumer` is null.
     - `f_reply_promise` is null.
     - `wait_for_reply()` returns immediately with a null `reply_ptr_t`.

     The destructor cancels `f_reply_consumer` if non-null, draining any in-flight reply messages.
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

            /*!
             @struct exchange_store
             @brief Bundles the rmqcpp topology, exchange handle, producer, and queue registry for a single AMQP exchange.

             @details
             One `exchange_store` is held by `core` for the requests exchange (`f_requests_ex`) and one for
             the alerts exchange (`f_alerts_ex`).  Both are populated lazily inside `open_connection()`.

             Usage:
             - `add_queue()` declares a durable queue on `f_topo` and records its handle in `f_queues`.
               Must be called after `open_connection()` (the exchange handle must already exist).
             - `bind_key()` binds an existing queue to `f_exchange` using the supplied routing key.
               It requires the named queue to already be present in `f_queues`.  The routing key is
               used verbatim — no wildcard suffixes are added; callers are responsible for their own
               patterns (e.g. `"my-service.#"`).

             Both `add_queue()` and `bind_key()` mutate `f_topo` in-place and must **not** be called
             while rmqcpp is actively using the topology object.
            */
            struct exchange_store
            {
                std::string f_name;  ///< Exchange name string (e.g. "requests" or "alerts")
                BloombergLP::rmqa::Topology f_topo;
                BloombergLP::rmqt::ExchangeHandle f_exchange;
                bsl::shared_ptr< BloombergLP::rmqa::Producer > f_producer;
                std::map< std::string, BloombergLP::rmqt::QueueHandle > f_queues;  ///< queue name → handle; used for duplicate-detection in bind_key()

                /// Declares a durable queue on this exchange's topology and records the handle.
                /// Returns the QueueHandle for use in bind_key() and start_listening().
                BloombergLP::rmqt::QueueHandle add_queue( const std::string& a_queue_name );

                /// Binds the named queue to this exchange with the given routing key.
                /// Throws connection_error if the queue was not previously added via add_queue().
                /// The routing key is used verbatim (no `.#` suffix is appended here).
                void bind_key( const std::string& a_queue_name, const std::string& a_routing_key, BloombergLP::rmqt::QueueHandle a_queue );
            };

            sent_msg_pkg_ptr do_send( message_ptr_t a_message, const std::string& a_exchange, bool a_expect_reply ) const;

            /// Sets up a temporary reply queue, starts an rmqcpp consumer on it, then sends the message.
            /// Stores the consumer and reply promise in a_pkg.
            void send_withreply( message_ptr_t a_message, const std::string& a_exchange, sent_msg_pkg_ptr a_pkg ) const;

            bool send_noreply( message_ptr_t a_message, const std::string& a_exchange ) const;

            /// Lazily establishes the RabbitMQ connection and creates the requests/alerts producers.
            /// Thread-safe (mutex-guarded); subsequent calls after the first successful connection are no-ops.
            void open_connection() const;

            /*!
             @brief Declares a durable queue on the **requests** exchange topology.
             @details
             Must be called after `open_connection()` (the requests exchange handle must exist).
             The returned QueueHandle must be stored (typically in `message_dispatcher::f_queue`) and
             later passed to `bind_requests_key()` and to `message_dispatcher::start_listening()`.
             @param a_queue_name  The unique name for this queue (usually the service or endpoint name).
             @return The QueueHandle for the newly declared queue.
            */
            BloombergLP::rmqt::QueueHandle add_requests_queue( const std::string& a_queue_name );

            /*!
             @brief Binds a queue on the **requests** exchange to the given routing key.
             @details
             Must be called after `add_requests_queue()` for the same queue name.
             The routing key is used verbatim; include any desired wildcard suffixes in `a_routing_key`
             (e.g. pass `"my-service.#"` to match all keys under `my-service`).
             @param a_queue_name   The queue name (must already exist in the exchange store).
             @param a_routing_key  The routing key pattern to bind.
             @param a_queue        The QueueHandle returned by `add_requests_queue()`.
            */
            void bind_requests_key( const std::string& a_queue_name, const std::string& a_routing_key, BloombergLP::rmqt::QueueHandle a_queue );

            /*!
             @brief Declares a durable queue on the **alerts** exchange topology.
             @details
             Must be called after `open_connection()` (the alerts exchange handle must exist).
             @param a_queue_name  The unique name for this queue.
             @return The QueueHandle for the newly declared queue.
            */
            BloombergLP::rmqt::QueueHandle add_alerts_queue( const std::string& a_queue_name );

            /*!
             @brief Binds a queue on the **alerts** exchange to the given routing key.
             @details
             Must be called after `add_alerts_queue()` for the same queue name.
             @param a_queue_name   The queue name (must already exist in the exchange store).
             @param a_routing_key  The routing key pattern to bind (verbatim; no suffix appended).
             @param a_queue        The QueueHandle returned by `add_alerts_queue()`.
            */
            void bind_alerts_key( const std::string& a_queue_name, const std::string& a_routing_key, BloombergLP::rmqt::QueueHandle a_queue );


            mutable bsl::shared_ptr< BloombergLP::rmqa::RabbitContext > f_rabbit_context;
            mutable bsl::shared_ptr< BloombergLP::rmqa::VHost > f_vhost;

            mutable exchange_store f_requests_ex;
            mutable exchange_store f_alerts_ex;

            mutable std::shared_ptr< std::mutex > f_connection_mutex;
    };

} /* namespace dripline */

#endif /* DRIPLINE_CORE_HH_ */
