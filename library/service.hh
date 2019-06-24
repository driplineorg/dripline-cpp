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

#include "dripline_error.hh"

#include "cancelable.hh"
#include "member_variables.hh"

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <set>
#include <thread>
#include <vector>

namespace dripline
{

    class DRIPLINE_API service : public core, public endpoint, public scarab::cancelable
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
                processing = 60
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

        public:
            /// Add a synchronous child endpoint
            bool add_child( endpoint_ptr_t a_endpoint_ptr );

            /// Add an asynchronous child endpoint
            bool add_asynch_child( endpoint_ptr_t a_endpoint_ptr );

        public:
            /// Sends a request message and returns a channel on which to listen for a reply.
            virtual rr_pkg_ptr send( request_ptr_t a_request ) const;

            /// Sends a reply message
            virtual bool send( reply_ptr_t a_reply ) const;

            /// Sends an alert message
            virtual bool send( alert_ptr_t a_alert ) const;

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
            bool setup_queues();

            bool bind_keys();

            bool start_consuming();

            bool stop_consuming();

            bool remove_queue();

            void listen_on_queue( const std::string& a_name );

            template< typename ptr_type >
            void do_on_message( ptr_type a_endpoint_ptr, message_ptr_t a_message );

        public:
            mv_referrable_const( amqp_channel_ptr, channel );

            mv_referrable_const( std::string, consumer_tag );

            typedef std::map< std::string, endpoint_ptr_t > child_map_t;
            mv_referrable( child_map_t, sync_children );
            mv_referrable( child_map_t, async_children );

            mv_referrable( std::string, broadcast_key );

            mv_accessible( unsigned, listen_timeout_ms );

            mv_referrable( std::vector< std::thread >, threads );
    };

    template< typename ptr_type >
    void service::do_on_message( ptr_type a_endpoint_ptr, message_ptr_t a_message )
    {
        if( a_message->is_request() )
        {
            a_endpoint_ptr->on_request_message( std::static_pointer_cast< msg_request >( a_message ) );
        }
        else if( a_message->is_alert() )
        {
            a_endpoint_ptr->on_alert_message( std::static_pointer_cast< msg_alert >( a_message ) );
        }
        else if( a_message->is_reply() )
        {
            a_endpoint_ptr->on_reply_message( std::static_pointer_cast< msg_reply >( a_message ) );
        }
        else
        {
            throw dripline_error() << "Unknown message type";
        }
    }

    inline bool service::add_child( endpoint_ptr_t a_endpoint_ptr )
    {
        auto t_inserted = f_sync_children.insert( std::make_pair( a_endpoint_ptr->name(), a_endpoint_ptr ) );
        return t_inserted.second;
    }

    inline bool service::add_asynch_child( endpoint_ptr_t a_endpoint_ptr )
    {
        auto t_inserted = f_async_children.insert( std::make_pair( a_endpoint_ptr->name(), a_endpoint_ptr ) );
        return t_inserted.second;
    }

    inline rr_pkg_ptr service::send( request_ptr_t a_request ) const
    {
        a_request->set_sender_service_name( f_name );
        return core::send( a_request );
    }

    inline bool service::send( reply_ptr_t a_reply ) const
    {
        a_reply->set_sender_service_name( f_name );
        return core::send( a_reply );
    }

    inline bool service::send( alert_ptr_t a_alert ) const
    {
        a_alert->set_sender_service_name( f_name );
        return core::send( a_alert );
    }

} /* namespace dripline */

#endif /* DRIPLINE_SERVICE_HH_ */
