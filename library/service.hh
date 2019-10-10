/*
 * service.hh
 *
 *  Created on: Jan 5, 2016
 *      Author: nsoblath
 */

#ifndef DRIPLINE_SERVICE_HH_
#define DRIPLINE_SERVICE_HH_

#include "core.hh"
#include "endpoint.hh"
#include "heartbeater.hh"
#include "scheduler.hh"
#include "listener.hh"
#include "receiver.hh"

#include "dripline_exceptions.hh"
#include "uuid.hh"

#include <map>
#include <memory>
#include <set>
#include <vector>

namespace dripline
{
    class DRIPLINE_API service :
            public core,
            public endpoint,
            public listener_receiver,
            public heartbeater,
            public scheduler<>,
            public std::enable_shared_from_this< service >
    {
        protected:
            enum class status
            {
                nothing = 0,
                channel_created = 10,
                exchange_declared = 20,
                queue_declared = 30,
                queue_bound = 40,
                consuming = 50,
                listening = 60,
                processing = 70
            };

        public:
            service( const scarab::param_node& a_config = scarab::param_node(), const std::string& a_queue_name = "",  const std::string& a_broker_address = "", unsigned a_port = 0, const std::string& a_auth_file = "", const bool a_make_connection = true );
            service( const bool a_make_connection, const scarab::param_node& a_config = scarab::param_node() );
            service( const service& ) = delete;
            service( service&& ) = delete;
            virtual ~service();

            service& operator=( const service& ) = delete;
            service& operator=( service&& ) = delete;

            mv_accessible( status, status );
            mv_accessible( bool, enable_scheduling );

        public:
            /// Add a synchronous child endpoint
            bool add_child( endpoint_ptr_t a_endpoint_ptr );

            /// Add an asynchronous child endpoint
            bool add_async_child( endpoint_ptr_t a_endpoint_ptr );

        public:
            /// Sends a request message and returns a channel on which to listen for a reply.
            virtual sent_msg_pkg_ptr send( request_ptr_t a_request ) const;

            /// Sends a reply message
            virtual sent_msg_pkg_ptr send( reply_ptr_t a_reply ) const;

            /// Sends an alert message
            virtual sent_msg_pkg_ptr send( alert_ptr_t a_alert ) const;

        public:
            /// Creates a channel to the broker and establishes the queue for receiving messages.
            /// If no queue name was given, this does nothing.
            bool start();

            /// Starts listening on the queue for receiving messages.
            /// If no queue was created, this does nothing.
            bool listen();

            /// Stops receiving messages and closes the connection to the broker.
            /// If no queue was created, this does nothing.
            bool stop();

        protected:
            bool open_channels();

            bool setup_queues();

            bool bind_keys();

            bool start_consuming();

            bool stop_consuming();

            bool remove_queue();

        public:
            virtual bool listen_on_queue();

            virtual void submit_message( message_ptr_t a_message );

            virtual void send_reply( reply_ptr_t a_reply ) const;

            mv_accessible( uuid_t, id );

        public:
            typedef std::map< std::string, endpoint_ptr_t > sync_map_t;
            mv_referrable( sync_map_t, sync_children );

            typedef std::map< std::string, lr_ptr_t > async_map_t;
            mv_referrable( async_map_t, async_children );

            mv_referrable( std::string, broadcast_key );

        private:
            virtual void do_cancellation( int a_code );
    };

    inline sent_msg_pkg_ptr service::send( request_ptr_t a_request ) const
    {
        a_request->sender_service_name() = f_name;
        return core::send( a_request );
    }

    inline sent_msg_pkg_ptr service::send( reply_ptr_t a_reply ) const
    {
        a_reply->sender_service_name() = f_name ;
        return core::send( a_reply );
    }

    inline sent_msg_pkg_ptr service::send( alert_ptr_t a_alert ) const
    {
        a_alert->sender_service_name() = f_name;
        return core::send( a_alert );
    }

} /* namespace dripline */

#endif /* DRIPLINE_SERVICE_HH_ */
