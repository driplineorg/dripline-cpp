/*
 * receiver.hh
 *
 *  Created on: Feb 18, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_RECEIVER_HH_
#define DRIPLINE_RECEIVER_HH_

#include "dripline_api.hh"
#include "dripline_fwd.hh"

#include "cancelable.hh"
#include "concurrent_queue.hh"
#include "member_variables.hh"

#include <atomic>
#include <condition_variable>
#include <map>
#include <thread>

namespace dripline
{

    struct incoming_message_pack
    {
        amqp_split_message_ptrs f_messages;
        unsigned f_chunks_received;
        std::string f_routing_key;
        std::thread f_thread;
        std::mutex f_mutex;
        std::condition_variable f_conv;
        std::atomic< bool > f_processing;
    };
    typedef std::map< std::string, incoming_message_pack > incoming_message_map;


    class DRIPLINE_API receiver
    {
        public:
            receiver();
            virtual ~receiver();

            mv_referrable( incoming_message_map, incoming_messages );

        public:
            /// Wait for a reply message
            /// If the timeout is <= 0 ms, there will be no timeout
            /// This function can be called multiple times to receive multiple replies
            /// The optional bool argument a_chan_valid will return whether or not the channel is still valid for use
            reply_ptr_t wait_for_reply( const sent_msg_pkg_ptr a_receive_reply, int a_timeout_ms = 0 );
            reply_ptr_t wait_for_reply( const sent_msg_pkg_ptr a_receive_reply, bool& a_chan_valid, int a_timeout_ms = 0 );

        protected:
            reply_ptr_t process_received_reply( incoming_message_pack& a_pack, const std::string& a_message_id );

    };

    class DRIPLINE_API receiver_parallel : public receiver, public virtual scarab::cancelable
    {
        public:
            receiver_parallel();
            virtual ~receiver_parallel();

        public:
            void execute();

        protected:
            // to be overridden by the thing inheriting from receiver_parallel
            virtual void submit_message( message_ptr_t a_message ) = 0;

            mv_referrable( scarab::concurrent_queue< message_ptr_t >, message_queue );
    };

} /* namespace dripline */

#endif /* DRIPLINE_RECEIVER_HH_ */
