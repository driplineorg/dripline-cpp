/*
 * receiver.hh
 *
 *  Created on: Feb 18, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_RECEIVER_HH_
#define DRIPLINE_RECEIVER_HH_

#include "core.hh"
#include "dripline_api.hh"
#include "dripline_fwd.hh"

#include "rmqa_topology.h"

#include "cancelable.hh"
#include "concurrent_queue.hh"
#include "member_variables.hh"

#include <atomic>
#include <condition_variable>
#include <map>
#include <thread>

namespace dripline
{

    /*!
     @struct incoming_message_pack
     @author N.S. Oblath
     @brief Stores the basic information about a set of message chunks that will eventually make a Dripline message
    */
    struct incoming_message_pack
    {
        amqp_split_message_ptrs f_messages;
        unsigned f_chunks_received;
        std::string f_routing_key;
        std::thread f_thread;
        std::mutex f_mutex;
        std::condition_variable f_conv;
        std::atomic< bool > f_processing;
        incoming_message_pack();
        incoming_message_pack( const incoming_message_pack& ) = delete;
        incoming_message_pack( incoming_message_pack&& a_orig );
    };
    typedef std::map< std::string, incoming_message_pack > incoming_message_map;


    // contains mechanisms for receiving messages synchronously
    /*!
     @class receiver
     @author N.S. Oblath

     @brief A receiver is able to collect Dripline message chunks and reassemble them into a complete Dripline message.

     @details
     This is a mix-in class for synchronously receiving and processing Dripline messages.

     Dripline messages can be broken up into multiple chunks, each of which is transported as an AMQP message.  
     A receiver is responsible for handling message chunks, storing incomplete Dripline messages, and eventually 
     processing complete Dripline messages.

     The receiver class contains an interface specifically for users waiting to receive reply messages: `wait_for_reply()`.

     When the first message chunk for a message is received, one of two things happens:
     1. if the message comprises one chunk, then the message is processed immediately;
     2. if the message comprises multiple chunks, then a separate thread is spun up to wait for the remaining chunks.

     Incomplete messages are stored in the incoming-message map.  Message chunks for a given message can be received 
     in any order.  The receiver will wait `single_message_wait_ms` ms for all of the chunks of a message to arrive 
     before timing out processing the incomplete message.

     The actual assembly of message chunks into complete messages is done in @ref message.

     The `receiver` class itself does not know how to process a message.  This must be implemented by the class derived from `receiver`.
     The default implementation of `process_message()` will throw a `dripline_error`.
    */
    class DRIPLINE_API receiver : public virtual scarab::cancelable
    {
        public:
            receiver();
            receiver( const receiver& a_orig ) = delete;
            receiver( receiver&& a_orig ) = default;
            virtual ~receiver() = default;

            receiver& operator=( const receiver& a_orig ) = delete;
            receiver& operator=( receiver&& a_orig );

        public:
            /// Processes a message chunk: starts a new message pack if it's the first of multiple messages, 
            /// or puts the chunk in the correct existing message pack.
            /// For single-chunk messages, processes the message immediately.
            void handle_message_chunk( amqp_envelope_ptr a_envelope );

            /// Waits for messages for a set amount of time (`single_message_wait_ms`), and submits the message pack for processing.
            /// Intended to be used in a separate thread for each message pack.
            void wait_for_message( incoming_message_pack& a_pack, const std::string& a_message_id );
            /// Converts a message pack into a Dripline message, and then submits the message for processing.
            void process_message_pack( incoming_message_pack& a_pack, const std::string& a_message_id );

            /// Processes a single Dripline message.
            /// This is the default implementation that always throws a `dripline_error`.
            virtual void process_message( message_ptr_t a_message );

            /// Stores the incomplete messages
            mv_referrable( incoming_message_map, incoming_messages );
            /// Wait time for all message chunks from a single dripline message
            mv_accessible( unsigned, single_message_wait_ms );


        public:
            /*!
            User interface for waiting for a reply message.
            This can be called multiple times to receive multiple replies.
            @param a_receive_reply The sent-message package from the request.
            @param a_timeout_ms Timeout for waiting for a reply; if it's 0, there will be no timeout.
            @return Reply message
            */
            reply_ptr_t wait_for_reply( const sent_msg_pkg_ptr a_receive_reply, int a_timeout_ms = 0 );

        protected:
            // (no protected helpers currently)

    };

    /*!
     @class concurrent_receiver
     @author N.S. Oblath

     @brief Receives and processes messages concurrently

     @details
     This class enables Dripline messages to be received and processed concurrently.

     The typical use case involves two threads:
     1. A consumer callback (set up via `start_listening()`) receives messages from the AMQP broker and
        calls `receiver::handle_message_chunk()`
     2. A concurrent_receiver picks up the complete message from the concurrent queue (via `execute()`), and processes the message using `submit_message()`.

     The `execute()` function implements thread 2.

     A class deriving from concurrent_receiver must implement `submit_message()`.
    */
    class DRIPLINE_API concurrent_receiver : public receiver
    {
        public:
            concurrent_receiver();
            concurrent_receiver( const concurrent_receiver& ) = delete;
            concurrent_receiver( concurrent_receiver&& a_orig );
            virtual ~concurrent_receiver();

            concurrent_receiver& operator=( const concurrent_receiver& ) = delete;
            concurrent_receiver& operator=( concurrent_receiver&& a_orig );

        public:
            /// Deposits the message in the concurrent queue (called by the listener)
            virtual void process_message( message_ptr_t a_message );

            /// Handles messages that appear in the concurrent queue by calling `submit_message()`.
            void execute();

            /// Creates an rmqcpp consumer on the given queue and begins receiving messages.
            /// Each received message is passed to handle_message_chunk().
            void start_listening( bsl::shared_ptr< BloombergLP::rmqa::VHost > a_vhost,
                                  const BloombergLP::rmqa::Topology& a_topology,
                                  const BloombergLP::rmqt::QueueHandle& a_queue_handle,
                                  const std::string& a_label = "" );

            /// Cancels the rmqcpp consumer, drains in-flight messages, and releases it.
            void stop_listening();

        protected:
            /// Handles messages according to the use case.  It's to be implemented by the class inheriting from concurrent_receiver
            /// For a concrete example, see @ref service or @ref endpoint_listener_receiver.
            virtual void submit_message( message_ptr_t a_message ) = 0;

            mv_referrable( scarab::concurrent_queue< message_ptr_t >, message_queue );
            mv_referrable( std::thread, receiver_thread );
            bsl::shared_ptr< BloombergLP::rmqa::Consumer > f_consumer;
    };

} /* namespace dripline */

#endif /* DRIPLINE_RECEIVER_HH_ */
