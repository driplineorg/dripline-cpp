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

#include "cancelable.hh"
#include "member_variables.hh"

#include <chrono>
#include <map>
#include <mutex>

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
        std::chrono::steady_clock::time_point f_creation_time;
        std::mutex f_mutex;
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
     This is a mix-in class for receiving and processing Dripline messages.

     Dripline messages can be broken up into multiple chunks, each of which is transported as an AMQP message.
     A receiver is responsible for handling message chunks, storing incomplete Dripline messages, and eventually
     processing complete Dripline messages.

     The receiver class contains an interface specifically for users waiting to receive reply messages: `wait_for_reply()`.

     When a message chunk arrives via `handle_message_chunk()`, it is stored in the incoming-message map.
     Message chunks for a given message can be received in any order.  Once all chunks for a message have
     arrived, `process_message_pack()` is called inline (no separate thread is spawned).

     Stale incomplete messages (entries older than `single_message_wait_ms` ms) are lazily evicted at the
     start of each `handle_message_chunk()` call.

     The actual assembly of message chunks into complete messages is done in @ref message.

     The `receiver` class itself does not know how to process a message.  This must be implemented by the
     class derived from `receiver`.  The default implementation of `process_message()` will throw a
     `dripline_error`.
    */
    class DRIPLINE_API receiver : public virtual scarab::cancelable
    {
        public:
            receiver();
            receiver( const receiver& a_orig ) = delete;
            receiver( receiver&& a_orig );
            virtual ~receiver() = default;

            receiver& operator=( const receiver& a_orig ) = delete;
            receiver& operator=( receiver&& a_orig );

        public:
            /// Processes a message chunk: starts a new message pack if it's the first of multiple messages, 
            /// or puts the chunk in the correct existing message pack.
            /// For single-chunk messages, processes the message immediately.
            void handle_message_chunk( amqp_envelope_ptr a_envelope );

            /// Converts a message pack into a Dripline message, and then submits the message for processing.
            void process_message_pack( amqp_split_message_ptrs& a_messages, const std::string& a_routing_key );

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

        private:
            mutable std::mutex f_incoming_messages_mutex;

    };

} /* namespace dripline */

#endif /* DRIPLINE_RECEIVER_HH_ */
