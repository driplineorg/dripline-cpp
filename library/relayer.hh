#ifndef DRIPLINE_AMQP_RELAYER_HH_
#define DRIPLINE_AMQP_RELAYER_HH_


#include "core.hh"
#include "receiver.hh"

#include "cancelable.hh"
#include "concurrent_queue.hh"

#include <condition_variable>
#include <mutex>

namespace scarab
{
    class param_node;
}

namespace dripline
{

    class DRIPLINE_API relayer : public core, public scarab::cancelable
    {
        public:
            relayer( const scarab::param_node& a_config = scarab::param_node(), const std::string& a_broker_address = "", unsigned a_port = 0, const std::string& a_auth_file = "" );
            virtual ~relayer();

        public:
            //*****************
            // thread functions
            //*****************

            /// main thread execution function: send any messages that are submitted via the send functions
            void execute_relayer();

        public:
            //********************************
            // asynchronous message submission
            //********************************

            struct wait_for_send_pkg
            {
                mutable std::mutex f_mutex;
                mutable std::condition_variable f_condition_var;
                sent_msg_pkg_ptr f_sent_msg_pkg_ptr;
            };
            typedef std::shared_ptr< wait_for_send_pkg > wait_for_send_pkg_ptr;

            wait_for_send_pkg_ptr send_async( request_ptr_t a_request ) const;
            wait_for_send_pkg_ptr send_async( alert_ptr_t a_alert ) const;

            /// Wait for a reply message
            /// If the timeout is <= 0 ms, there will be no timeout
            /// This function can be called multiple times to receive multiple replies
            /// The optional bool argument a_chan_valid will return whether or not the channel is still valid for use
            reply_ptr_t wait_for_reply( const wait_for_send_pkg_ptr a_receive_reply, int a_timeout_ms = 0 );
            reply_ptr_t wait_for_reply( const wait_for_send_pkg_ptr a_receive_reply, bool& a_chan_valid, int a_timeout_ms = 0 );

        private:
            void do_cancellation();

            struct message_and_reply
            {
                message_ptr_t f_message;
                wait_for_send_pkg_ptr f_wait_for_send_pkg;
            };
            typedef std::shared_ptr< message_and_reply > mar_ptr;

            mutable scarab::concurrent_queue< mar_ptr > f_queue;

            mv_referrable( receiver, msg_receiver );

    };

}

#endif
