/*
 * message_dispatcher.hh
 *
 *  Created on: Jun 3, 2026
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_MESSAGE_DISPATCHER_HH_
#define DRIPLINE_MESSAGE_DISPATCHER_HH_

#include "receiver.hh"

#include "rmqa_topology.h"

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

     The topology and queue for this dispatcher are populated by `service::add_queues()`
     (which sets `f_queue` via `core::add_requests_queue()` and builds `f_topology` with
     the exchange + queue declaration) and `service::bind_keys()` (which adds bindings to
     `f_topology`).  Both must be called before `start_listening()`.

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
             @brief Creates an rmqcpp consumer using this dispatcher's own topology and queue,
                    then begins receiving messages.

             @details
             `f_topology` and `f_queue` must be fully populated before calling this method.
             `f_topology` is built by `service::add_queues()` (exchange + queue declaration) and
             `service::bind_keys()` (bindings).  `f_queue` is set by `service::add_queues()` via
             `core::add_requests_queue()`.

             Throws `connection_error` if the rmqcpp consumer cannot be created.

             @param a_vhost  The rmqcpp VHost to create the consumer on.
             @param a_label  Optional label for the consumer (used for logging/identification).
            */
            void start_listening( bsl::shared_ptr< BloombergLP::rmqa::VHost > a_vhost,
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
            /// Set by `service::add_queues()` (via `core::add_requests_queue()`) before
            /// `start_listening()` is called.
            BloombergLP::rmqt::QueueHandle f_queue;

            /// The topology that declares the queue and its bindings for this dispatcher.
            /// Populated by `service::add_queues()` (exchange + queue) and
            /// `service::bind_keys()` (bindings).  Passed to rmqcpp's `createConsumer()`
            /// inside `start_listening()` so that the broker can redeclare the topology
            /// after a connection restart.
            BloombergLP::rmqa::Topology f_topology;


    };

} /* namespace dripline */

#endif /* DRIPLINE_MESSAGE_DISPATCHER_HH_ */
