#ifndef DRIPLINE_AMQP_RELAYER_HH_
#define DRIPLINE_AMQP_RELAYER_HH_


#include "core.hh"

#include "cancelable.hh"
#include "concurrent_queue.hh"

//#include <boost/date_time/posix_time/posix_time.hpp>

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
            relayer( const scarab::param_node* a_config = nullptr, const std::string& a_broker_address = "", unsigned a_port = 0, const std::string& a_auth_file = "" );
            virtual ~relayer();

        public:
            //*****************
            // thread functions
            //*****************

            /// main thread execution function: send any messages that are submitted via the send functions
            void execute_relayer();

            /// asynchronous cancel
            void cancel_relayer();

        public:
            //********************************
            // asynchronous message submission
            //********************************

            struct cc_receive_reply_pkg : receive_reply_pkg
            {
                mutable std::mutex f_mutex;
                mutable std::condition_variable f_condition_var;
                cc_receive_reply_pkg& operator=( const receive_reply_pkg& a_orig )
                {
                    // not thread safe
                    f_channel = a_orig.f_channel;
                    f_consumer_tag = a_orig.f_consumer_tag;
                    f_successful_send = a_orig.f_successful_send;
                    return *this;
                }
            };
            typedef std::shared_ptr< cc_receive_reply_pkg > cc_rr_pkg_ptr;

            cc_rr_pkg_ptr send_async( request_ptr_t a_request );
            bool send_async( alert_ptr_t a_alert );

            /// Wait for a reply message
            /// If the timeout is <= 0 ms, there will be no timeout
            /// This function can be called multiple times to receive multiple replies
            /// The optional bool argument a_chan_valid will return whether or not the channel is still valid for use
            static reply_ptr_t wait_for_reply( const cc_rr_pkg_ptr a_receive_reply, int a_timeout_ms = 0 );
            static reply_ptr_t wait_for_reply( const cc_rr_pkg_ptr a_receive_reply, bool& a_chan_valid, int a_timeout_ms = 0 );

        private:
            void do_cancellation();

            struct message_and_reply
            {
                message_ptr_t f_message;
                cc_rr_pkg_ptr f_receive_reply;
            };
            typedef std::shared_ptr< message_and_reply > mar_ptr;

            scarab::concurrent_queue< mar_ptr > f_queue;

    };

}

#endif
