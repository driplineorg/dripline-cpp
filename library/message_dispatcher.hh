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

            /// Creates an rmqcpp consumer on the given queue and begins receiving messages.
            /// Each received message is passed to handle_message_chunk().
            void start_listening( bsl::shared_ptr< BloombergLP::rmqa::VHost > a_vhost,
                                  const BloombergLP::rmqa::Topology& a_topology,
                                  const BloombergLP::rmqt::QueueHandle& a_queue_handle,
                                  const std::string& a_label = "" );

            /// Cancels the rmqcpp consumer, drains in-flight messages, and releases it.
            void stop_listening();

            /// Handles messages according to the use case.  Must be implemented by the
            /// class inheriting from message_dispatcher.
            /// For a concrete example, see @ref service or @ref endpoint_listener_receiver.
            virtual void submit_message( message_ptr_t a_message ) = 0;

        protected:
            bsl::shared_ptr< BloombergLP::rmqa::Consumer > f_consumer;
    };

} /* namespace dripline */

#endif /* DRIPLINE_MESSAGE_DISPATCHER_HH_ */
