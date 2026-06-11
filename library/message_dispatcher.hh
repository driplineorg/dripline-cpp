/*
 * message_dispatcher.hh
 *
 *  Created on: Jun 3, 2026
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_MESSAGE_DISPATCHER_HH_
#define DRIPLINE_MESSAGE_DISPATCHER_HH_

#include "receiver.hh"

namespace BloombergLP { namespace rmqa { class Topology; } }

namespace dripline
{

    /*!
     @class message_dispatcher
     @author N.S. Oblath

     @brief Receives assembled Dripline messages from an AMQP consumer and dispatches them.

     @details
     This class manages the lifecycle of an rmqcpp Consumer (via `start_listening()` /
     `stop_listening()`) and overrides `process_message()` to dispatch each assembled
     Dripline message directly to `submit_message()` (synchronously in the rmqcpp
     callback thread).

     A class deriving from `message_dispatcher` must implement `submit_message()` to
     define what happens with each received message.

     **Topology ownership**

     `message_dispatcher` does **not** own the AMQP topology.  The topology is owned by
     the `core` instance that the concrete subclass (e.g. `service`, `monitor`) also
     inherits from.  The queue handle `f_queue` is set by the subclass via
     `core::add_requests_ephemeral_queue()` (or the durable variant) and then the shared
     `core::f_topology` is passed to `start_listening()`.  This means that after a
     connection restart rmqcpp will redeclare the complete topology — exchanges, queues,
     and bindings — from the single canonical object in `core`.

     The typical call sequence (performed by `service`) is:
     1. `open_connection()` — establishes the connection and declares exchanges.
     2. `add_queues()` — calls `core::add_requests_ephemeral_queue()` → stores result in `f_queue`.
     3. `bind_keys()` — calls `core::bind_requests_key()` with `f_queue`.
     4. `start_listening( f_vhost, core::f_topology, label )` — starts consuming.

     @note
     The name `concurrent_receiver` was used for this class prior to the rmqcpp migration.
     After the migration the class no longer manages any concurrency itself — rmqcpp's
     internal thread pool handles delivery — so it was renamed to `message_dispatcher`.
    */
    class DRIPLINE_API message_dispatcher : public receiver
    {
        public:
            message_dispatcher();
            message_dispatcher( const message_dispatcher& ) = delete;
            message_dispatcher( message_dispatcher&& a_orig );
            virtual ~message_dispatcher();

            message_dispatcher& operator=( const message_dispatcher& ) = delete;
            message_dispatcher& operator=( message_dispatcher&& a_orig );

        public:
            /// Dispatches the message directly to `submit_message()`.
            virtual void process_message( message_ptr_t a_message );

            /*!
             @brief Creates an rmqcpp consumer using the supplied topology and this dispatcher's
                    queue handle, then begins receiving messages.

             @details
             `f_queue` must be set (via `core::add_requests_ephemeral_queue()` or similar) and
             the bindings must be added to `a_topology` (via `core::bind_requests_key()` etc.)
             before calling this method.

             The topology is passed by (non-const) reference because rmqcpp requires it that
             way; the underlying `Topology` object is owned by `core::f_topology`.

             Throws `connection_error` if the rmqcpp consumer cannot be created.

             @param a_vhost     The rmqcpp VHost to create the consumer on.
             @param a_topology  The shared topology (from `core::f_topology`) that rmqcpp uses
                                to redeclare exchanges, queues, and bindings after reconnects.
             @param a_label     Optional label for the consumer (used for logging/identification).
            */
            void start_listening( bsl::shared_ptr< BloombergLP::rmqa::VHost > a_vhost,
                                  BloombergLP::rmqa::Topology& a_topology,
                                  const std::string& a_label = "" );

            /*!
             @brief Cancels the rmqcpp consumer, drains in-flight messages, and releases it.

             @details
             This method is idempotent: it is safe to call even if `start_listening()` was never
             called or if the consumer has already been stopped.
            */
            void stop_listening();

            /// Handles messages according to the use case.  Must be implemented by the
            /// class inheriting from message_dispatcher.
            /// For a concrete example, see @ref service or @ref endpoint_listener_receiver.
            virtual void submit_message( message_ptr_t a_message ) = 0;

        protected:
            bsl::shared_ptr< BloombergLP::rmqa::Consumer > f_consumer;

            /// The queue this dispatcher consumes from.
            /// Set by the concrete subclass (e.g. via `service::add_queues()` calling
            /// `core::add_requests_ephemeral_queue()`) before `start_listening()` is called.
            BloombergLP::rmqt::QueueHandle f_queue;
    };

} /* namespace dripline */

#endif /* DRIPLINE_MESSAGE_DISPATCHER_HH_ */
