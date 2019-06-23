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

namespace dripline
{

    class DRIPLINE_API service : public core, public endpoint, public scarab::cancelable
    {
        public:
            service( const scarab::param_node& a_config = scarab::param_node(), const std::string& a_queue_name = "",  const std::string& a_broker_address = "", unsigned a_port = 0, const std::string& a_auth_file = "", const bool a_make_connection = true );
            service( const bool a_make_connection, const scarab::param_node& a_config = scarab::param_node() );
            service( const service& ) = delete;
            service( service&& ) = delete;
            virtual ~service();

            service& operator=( const service& ) = delete;
            service& operator=( service&& ) = delete;

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

        private:
            bool bind_keys();

            bool start_consuming();

            bool stop_consuming();

            bool remove_queue();

        public:
            mv_referrable_const( amqp_channel_ptr, channel );

            mv_referrable_const( std::string, consumer_tag );

            typedef std::map< std::string, endpoint_ptr_t > child_map_t;
            mv_referrable( child_map_t, children );

            mv_referrable( std::string, broadcast_key );

            mv_accessible( unsigned, listen_timeout_ms );

        protected:
            template< typename ptr_type >
            void do_on_message( ptr_type a_endpoint_ptr, message_ptr_t a_message );

        public:
            //******************
            // Lockout functions
            //******************

            /// enable lockout with randomly-generated key
            uuid_t enable_lockout( const scarab::param_node& a_tag );
            /// enable lockout with user-supplied key
            uuid_t enable_lockout( const scarab::param_node& a_tag, uuid_t a_key );
            bool disable_lockout( const uuid_t& a_key, bool a_force = false );

        private:
            friend class endpoint;

            /// Returns true if the server is unlocked or if it's locked and the key matches the lockout key; returns false otherwise.
            bool authenticate( const uuid_t& a_key ) const;

            scarab::param_node f_lockout_tag;
            uuid_t f_lockout_key;

        private:
            //*****************
            // Request handlers
            //*****************

            reply_ptr_t handle_lock_request( const request_ptr_t a_request );
            reply_ptr_t handle_unlock_request( const request_ptr_t a_request );
            reply_ptr_t handle_is_locked_request( const request_ptr_t a_request );
            reply_ptr_t handle_set_condition_request( const request_ptr_t a_request );

        private:
            /// Default set-condition: no action taken; override for different behavior
            virtual reply_ptr_t __do_handle_set_condition_request( const request_ptr_t a_request );
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

    inline uuid_t service::enable_lockout( const scarab::param_node& a_tag )
    {
        return enable_lockout( a_tag, generate_random_uuid() );
    }

    inline reply_ptr_t service::__do_handle_set_condition_request( const request_ptr_t a_request )
    {
        return a_request->reply( dl_success(), "No action taken (this is the default method)" );
    }

} /* namespace dripline */

#endif /* DRIPLINE_SERVICE_HH_ */
