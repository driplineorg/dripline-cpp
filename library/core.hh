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

#include "param_helpers_impl.hh"

#include "rmqa_topology.h"
#include "rmqt_exchange.h"

#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace scarab
{
    class authentication;
    class param_node;
}

namespace BloombergLP { 
    namespace rmqa { class Consumer; class RabbitContext; class VHost; class Producer; }
    namespace rmqt 
    { 
        class Queue; 
        typedef bsl::weak_ptr<Queue> QueueHandle;
    } 
}

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

     @brief Basic AMQP interactions, including sending messages and setting up the AMQP topology.

     @details
     The configuration for a `core` object is supplied via the constructor.  The basic required
     information can be obtained from `dripline_config`.  Configuration values have defaults that
     can be overridden by the supplied `param_node`; a few can also be set explicitly as constructor
     arguments.  Precedence (highest first):
       * Constructor arguments (other than `a_config`)
       * Config `param_node` object
       * Defaults

     If the broker address is not specified in either the config or as a constructor parameter, it
     will be requested from the authentication file.

     **Topology management**

     `core` owns a single `rmqa::Topology` (`f_topology`, accessible read-only via `topology()`).
     All AMQP queue and binding declarations for the service, monitor, or any other consumer must
     be made through `core`'s helper functions so that they are recorded in `f_topology`.
     `open_connection()` populates `f_topology` with both exchange declarations; subclasses then
     add queues and bindings on top of that.  The typical call sequence is:

     1. `open_connection()` — establishes the broker connection and declares the two exchanges in
        `f_topology`.
     2. `add_requests_[durable|ephemeral]_queue()` / `add_alerts_[durable|ephemeral]_queue()` —
        adds a queue declaration to `f_topology` and returns a `QueueHandle`.
     3. `bind_requests_key()` / `bind_alerts_key()` — adds a binding to `f_topology`.
     4. Pass `f_topology` to `message_dispatcher::start_listening()` so that rmqcpp can redeclare
        the full topology after a connection restart.

     **Durable vs. ephemeral queues**

     - *Durable* queues (`add_*_durable_queue()`) survive broker restarts.  Use these when the
       consumer needs to receive messages that arrived while it was offline.
     - *Ephemeral* queues (`add_*_ephemeral_queue()`) are auto-delete and non-durable: they
       disappear when the last consumer disconnects and are not restored after a broker restart.
       `service` and `monitor` both use ephemeral queues because they process messages in real
       time and have no need to buffer missed traffic.

     **Sending messages**

     The primary user interface is `core::send()`, one of which exists for each message type
     (request, reply, and alert).  Classes wishing to take advantage of the AMQP helpers should
     inherit from `core`.
    */
    class DRIPLINE_API core
    {
        public:
            static bool s_offline;

        public:
            /*!
               @brief Extracts configuration and authentication information; prepares this object
                      to interact with the RabbitMQ broker.  Does **not** open the connection.

               @param a_config Dripline configuration object.  Recognised keys:
                 - `broker` (string; default: localhost) — broker address
                 - `broker_port` (int; default: 5672) — broker port
                 - `requests_exchange` (string; default: requests) — exchange for DL requests
                 - `alerts_exchange` (string; default: alerts) — exchange for DL alerts
                 - `heartbeat_routing_key` (string; default: heartbeat) — heartbeat routing key
                 - `make_connection` (bool; default: true) — if false, operates in dry-run mode
                 - `max_payload_size` (int; default: DL_MAX_PAYLOAD_SIZE) — max payload in bytes
                 - `max_connection_attempts` (int; default: 10) — connection attempt limit
                 - `return_codes` (string or array of nodes) — optional extra return codes as
                        `[{name: "<name>", value: <ret code>, description: "<desc>"}, ...]`.
                        A string value is treated as a file path to a YAML/JSON file.
               @param a_auth  Authentication object; should contain `dripline/username` and
                              `dripline/password` (defaults to `guest`/`guest`).
               @param a_make_connection  If false, operates in dry-run mode (overridden by config).
             */
            core( const scarab::param_node& a_config = dripline_config(), const scarab::authentication& a_auth = scarab::authentication(), const bool a_make_connection = true );
            core( const core& a_orig ) = default;
            core( core&& a_orig ) = default;
            virtual ~core() = default;

            core& operator=( const core& a_orig ) = default;
            core& operator=( core&& a_orig ) = default;

        public:
            /// Sends a request message; creates a temporary auto-delete reply queue and waits
            /// for the reply via an rmqcpp consumer.  Default exchange is "requests".
            virtual sent_msg_pkg_ptr send( request_ptr_t a_request ) const;

            /// Sends a reply message.  Default exchange is "requests".
            virtual sent_msg_pkg_ptr send( reply_ptr_t a_reply ) const;

            /// Sends an alert message.  Default exchange is "alerts".
            virtual sent_msg_pkg_ptr send( alert_ptr_t a_alert ) const;

            const std::string& requests_exchange() const;
            std::string& requests_exchange();

            const std::string& alerts_exchange() const;
            std::string& alerts_exchange();

            mv_referrable( std::string, address );
            mv_accessible( unsigned, port );
            mv_referrable( std::string, username );
            mv_referrable( std::string, password );

            mv_referrable( std::string, heartbeat_routing_key );

            mv_accessible( unsigned, max_payload_size );

            mv_accessible( bool, make_connection );
            mv_accessible( unsigned, max_connection_attempts );

        public:
            /*!
             @brief Lazily establishes the RabbitMQ connection, declares both exchanges in
                    `f_topology`, and creates the requests and alerts producers.
             @details Thread-safe (mutex-guarded); subsequent calls after the first successful
                      connection are no-ops.  `f_topology` is populated here; all queue and
                      binding helpers must be called **after** this function.
            */
            void open_connection() const;

            /*!
             @brief Declares a **durable** queue on the requests exchange topology.
             @details Must be called after `open_connection()`.  Durable queues survive broker
                      restarts and buffer messages while the consumer is offline.  Use for
                      consumers that must not miss traffic across restarts.
             @param a_queue_name  Unique name for this queue (e.g. a service or endpoint name).
             @return QueueHandle to pass to `bind_requests_key()` and `message_dispatcher::start_listening()`.
            */
            BloombergLP::rmqt::QueueHandle add_requests_queue( const std::string& a_queue_name, 
                bool a_auto_delete=false, bool a_durable=true, 
                const scarab::param_node& a_field_table=scarab::param_node(scarab::kwarg("x-single-active-consumer")=true) );

            /*!
             @brief Declares an **ephemeral** (auto-delete, non-durable) queue on the requests exchange topology.
             @details Must be called after `open_connection()`.  Ephemeral queues are removed when
                      the consumer disconnects and are not restored after a broker restart.
                      `service` uses this so that stale messages from a previous run are discarded.
             @param a_queue_name  Unique name for this queue (typically the service name).
             @return QueueHandle to pass to `bind_requests_key()` and `message_dispatcher::start_listening()`.
            */
            //BloombergLP::rmqt::QueueHandle add_requests_ephemeral_queue( const std::string& a_queue_name );

            /*!
             @brief Binds a queue on the requests exchange to the given routing key.
             @details Must be called after `add_requests_durable_queue()` or
                      `add_requests_ephemeral_queue()` for the same queue.  The routing key is
                      used verbatim — append `.#` yourself if you want wildcard matching.
             @param a_queue_name   Queue name (used in log messages).
             @param a_routing_key  Routing key pattern to bind.
             @param a_queue        QueueHandle returned by the corresponding `add_requests_*_queue()`.
            */
            void bind_requests_key( const std::string& a_queue_name, const std::string& a_routing_key, BloombergLP::rmqt::QueueHandle a_queue );

            /*!
             @brief Declares a **durable** queue on the alerts exchange topology.
             @details Must be called after `open_connection()`.
             @param a_queue_name  Unique name for this queue.
             @return QueueHandle to pass to `bind_alerts_key()` and `message_dispatcher::start_listening()`.
            */
            BloombergLP::rmqt::QueueHandle add_alerts_queue( const std::string& a_queue_name, 
                bool a_auto_delete=false, bool a_durable=true, 
                const scarab::param_node& a_field_table=scarab::param_node(scarab::kwarg("x-single-active-consumer")=true) );

            /*!
             @brief Declares an **ephemeral** (auto-delete, non-durable) queue on the alerts exchange topology.
             @details Must be called after `open_connection()`.
             @param a_queue_name  Unique name for this queue.
             @return QueueHandle to pass to `bind_alerts_key()` and `message_dispatcher::start_listening()`.
            */
            //BloombergLP::rmqt::QueueHandle add_alerts_ephemeral_queue( const std::string& a_queue_name );

            /*!
             @brief Binds a queue on the alerts exchange to the given routing key.
             @details The queue handle may come from either `add_alerts_*_queue()` **or** from
                      `add_requests_*_queue()` — `monitor` intentionally binds a single queue to
                      both exchanges by calling both `bind_requests_key()` and `bind_alerts_key()`
                      with the same handle.
             @param a_queue_name   Queue name (used in log messages).
             @param a_routing_key  Routing key pattern to bind (verbatim).
             @param a_queue        QueueHandle to bind.
            */
            void bind_alerts_key( const std::string& a_queue_name, const std::string& a_routing_key, BloombergLP::rmqt::QueueHandle a_queue );

        protected:
            friend class receiver;

            /*!
             @struct exchange_store
             @brief Bundles the rmqcpp exchange handle and producer for a single AMQP exchange.

             @details
             One `exchange_store` is held by `core` for the requests exchange (`f_requests_ex`)
             and one for the alerts exchange (`f_alerts_ex`).  Both are populated lazily inside
             `open_connection()`.

             Queue and binding declarations are made directly on `core::f_topology` (the single
             shared topology), passed in by the `core` helpers.  `exchange_store` no longer owns
             a topology object.

             The helper methods `add_durable_queue()`, `add_ephemeral_queue()`, and `bind_key()`
             must only be called after `open_connection()` has populated `f_exchange`.
            */
            struct exchange_store
            {
                std::string f_name;  ///< Exchange name (e.g. "requests" or "alerts")
                BloombergLP::rmqt::ExchangeHandle f_exchange;
                bsl::shared_ptr< BloombergLP::rmqa::Producer > f_producer;

                /*!
                 @brief Declares a durable (non-auto-delete) queue on the supplied topology.
                 @param a_topo        The shared topology owned by `core`.
                 @param a_queue_name  Unique name for the queue.
                 @return Handle to the newly declared queue.
                */
                BloombergLP::rmqt::QueueHandle add_queue( BloombergLP::rmqa::Topology& a_topo, const std::string& a_queue_name, 
                    bool a_auto_delete, bool a_durable, 
                    const scarab::param_node& a_field_table );

                /*!
                 @brief Declares an ephemeral (auto-delete, non-durable) queue on the supplied topology.
                 @details Ephemeral queues are removed from the broker when the last consumer
                          disconnects and are not restored after a broker restart.  They are
                          appropriate for real-time consumers such as `service` and `monitor`.
                 @param a_topo        The shared topology owned by `core`.
                 @param a_queue_name  Unique name for the queue (typically includes a UUID or service name).
                 @return Handle to the newly declared queue.
                */
                //BloombergLP::rmqt::QueueHandle add_ephemeral_queue( BloombergLP::rmqa::Topology& a_topo, const std::string& a_queue_name );

                /*!
                 @brief Binds a queue to this exchange with the supplied routing key.
                 @details The routing key is used verbatim; include any desired wildcard suffixes
                          (e.g. `"my-service.#"`).  The queue handle may come from either
                          `add_durable_queue()` or `add_ephemeral_queue()` on **any** exchange
                          store — this is intentional, since `monitor` binds a single queue to
                          both the requests and the alerts exchange.
                 @param a_topo         The shared topology owned by `core`.
                 @param a_queue_name   Queue name (informational; used in error messages).
                 @param a_routing_key  Routing key pattern to bind (verbatim).
                 @param a_queue        Queue handle to bind.
                */
                void bind_key( BloombergLP::rmqa::Topology& a_topo, const std::string& a_queue_name, const std::string& a_routing_key, BloombergLP::rmqt::QueueHandle a_queue );
            };

            sent_msg_pkg_ptr do_send( message_ptr_t a_message, const std::string& a_exchange, bool a_expect_reply ) const;

            /// Sets up a temporary reply queue, starts an rmqcpp consumer on it, then sends the
            /// message.  Stores the consumer and reply promise in `a_pkg`.
            void send_withreply( message_ptr_t a_message, const std::string& a_exchange, sent_msg_pkg_ptr a_pkg ) const;

            bool send_noreply( message_ptr_t a_message, const std::string& a_exchange ) const;

            mutable bsl::shared_ptr< BloombergLP::rmqa::RabbitContext > f_rabbit_context;
            mutable bsl::shared_ptr< BloombergLP::rmqa::VHost > f_vhost;

            mutable exchange_store f_requests_ex;
            mutable exchange_store f_alerts_ex;

            /// The single shared AMQP topology for this core object.
            /// Populated by `open_connection()` (exchange declarations) and then by the
            /// `add_*_queue()` / `bind_*_key()` helpers.  Passed read-only via `topology()`.
            /// Passed to `message_dispatcher::start_listening()` so that rmqcpp can redeclare
            /// the full topology after a connection restart.
            mutable BloombergLP::rmqa::Topology f_topology;

            mutable std::shared_ptr< std::mutex > f_connection_mutex;

        public:
            /// Read-only access to the shared AMQP topology.
            /// Concrete subclasses (e.g. `service`, `monitor`) pass this to
            /// `message_dispatcher::start_listening()`.
            const BloombergLP::rmqa::Topology& topology() const { return f_topology; }
    };

    inline const std::string& core::requests_exchange() const
    {
        return f_requests_ex.f_name;
    }

    inline std::string& core::requests_exchange()
    {
        return f_requests_ex.f_name;
    }

    inline const std::string& core::alerts_exchange() const
    {
        return f_alerts_ex.f_name;
    }

    inline std::string& core::alerts_exchange()
    {
        return f_alerts_ex.f_name;
    }

} /* namespace dripline */

#endif /* DRIPLINE_CORE_HH_ */
