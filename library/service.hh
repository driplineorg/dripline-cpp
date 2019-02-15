/*
 * service.hh
 *
 *  Created on: Jan 5, 2016
 *      Author: nsoblath
 */

#ifndef DRIPLINE_SERVICE_HH_
#define DRIPLINE_SERVICE_HH_

#include "amqp.hh"
#include "core.hh"
#include "endpoint.hh"

#include "cancelable.hh"
#include "member_variables.hh"

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <set>
#include <thread>

namespace dripline
{

    class DRIPLINE_API service : public core, public endpoint, public scarab::cancelable
    {
        public:
            struct message_pack
            {
                amqp_split_message_ptrs f_messages;
                unsigned f_chunks_received;
                std::string f_routing_key;
                std::thread f_thread;
                std::mutex f_mutex;
                std::condition_variable f_conv;
                std::atomic< bool > f_processing;
            };
            typedef std::map< std::string, message_pack > message_map;

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

        private:
            bool bind_keys( const std::set< std::string >& a_keys );

            bool start_consuming();

            bool stop_consuming();

            bool remove_queue();

            void wait_for_message( message_pack& a_pack );
            void process_message( message_pack& a_pack );

        public:
            mv_referrable_const( amqp_channel_ptr, channel );

            mv_referrable_const( std::string, consumer_tag );

            mv_referrable( std::set< std::string >, keys );
            mv_referrable( std::string, broadcast_key );

            mv_referrable_const( message_map, incoming_messages );
            mv_accessible( unsigned, listen_timeout_ms );

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
