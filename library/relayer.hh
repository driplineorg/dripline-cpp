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

    /*!
     @class relayer
     @author N.S. Oblath

     @brief Asynchronous message sending

     @details
     This class allows a user to send message asynchronously.  Messages are sent in a dedicated thread that runs 
     the function `execute_relayer()`.  Asynchronous operation is achieved using a concurrent queue to 
     store messages that are submitted.

     The primary user interface consists of `send_async()` and `wait_for_reply`().  The former immediately returns a special 
     sent-message package that includes thread-synchonization objects.  That package is then passed to the latter,
     which blocks while waiting for a reply.
    */
    class DRIPLINE_API relayer : public core, public scarab::cancelable
    {
        public:
            relayer( const scarab::param_node& a_config = scarab::param_node(), const std::string& a_broker_address = "", unsigned a_port = 0, const std::string& a_auth_file = "" );
            virtual ~relayer();

        public:
            //*****************
            // thread functions
            //*****************

            /// Thread execution function: sends any messages that are submitted via the send functions
            void execute_relayer();

        public:
            //********************************
            // asynchronous message submission
            //********************************

            /*!
             @struct wait_for_send_pkg
             @author N.S. Oblath
             @brief Extended sent-message package that adds thread synchronization objects
            */
            struct wait_for_send_pkg
            {
                mutable std::mutex f_mutex;
                mutable std::condition_variable f_condition_var;
                sent_msg_pkg_ptr f_sent_msg_pkg_ptr;
            };
            typedef std::shared_ptr< wait_for_send_pkg > wait_for_send_pkg_ptr;

            /// Asynchronously send a request message
            /// Returns immediately, without blocking for send
            wait_for_send_pkg_ptr send_async( request_ptr_t a_request ) const;
            /// Asynchronously send an alert message
            /// Returns immediately, without blocking for send
            wait_for_send_pkg_ptr send_async( alert_ptr_t a_alert ) const;

            /*!
            User interface for waiting for a reply message.
            This can be called multiple times to receive multiple replies.
            @param a_receive_reply The sent-message package from the request.
            @param a_timeout_ms Timeout for waiting for a reply; if it's 0, there will be no timeout.
            @return Reply message
            */
            reply_ptr_t wait_for_reply( const wait_for_send_pkg_ptr a_receive_reply, int a_timeout_ms = 0 );
            /*!
            User interface for waiting for a reply message.
            This can be called multiple times to receive multiple replies.
            @param[in] a_receive_reply The sent-message package from the request.
            @param[in] a_timeout_ms Timeout for waiting for a reply; if it's 0, there will be no timeout.
            @param[out] a_status Returns the output status after receiving a message (or failing to do so).
            @return Reply message
            */
            reply_ptr_t wait_for_reply( const wait_for_send_pkg_ptr a_receive_reply, core::post_listen_status& a_status, int a_timeout_ms = 0 );

        private:
            void do_cancellation( int a_code );

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
