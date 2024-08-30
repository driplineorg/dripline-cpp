/*
 * service.hh
 *
 *  Created on: Jan 5, 2016
 *      Author: N.S. Oblath
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

namespace scarab
{
    class authentication;
}
namespace dripline
{

    /*!
     @class service
     @author N.S. Oblath

     @brief Primary unit of software that connects to a broker and typically provides an interface with an instrument or other software.

     @details
     The service class is the implementation of the "service" concept in Dripline.
     It's the primary component that makes up a Dripline mesh.

     The lifetime of a service is defined by the three main functions:
       1. `start()` -- create the AMQP channel, create the AMQP queue, bind the routing keys, and start consuming AMQP messages
       2. `listen()` -- starts the heartbeat and scheduler threads (optional), starts the receiver thread, and waits for and handles messages on the queue
       3. `stop()` -- (called asynchronously) cancels the listening service

     The ability to handle and respond to Dripline messages is embodied in the `endpoint` class.  
     Service uses `endoint` in three ways:
       1. Service is an endpoint.  A service can be setup to handle messages directed to it.
       2. Service has basic child endpoints.  These are also called "synchronous" endpoints.  
          These endpoints use the same AMQP queue as the service itself.  Messages send to the 
          service and to the synchronous endpoints are all handled serially.
       3. Service has asynchronous child endpoints.  These endpoints each have their own AMQP 
          queue and thread responsible for receiving and handling their messages.

     A service has a number of key characteristics (most of which come from its parent classes):
       * `core` -- Has all of the basic AMQP capabilities, sending messages, and making and manipulating connections
       * `endpoint` -- Handles Dripline messages
       * `listener_receiver` -- Asynchronously recieves AMQP messages and turns them into Dripline messages
       * `heartbeater` -- Sends periodic heartbeat messages
       * `scheduler` -- Can schedule events
    
     As is apparent from the above descriptions, a service is responsible for a number of threads 
     when it executes:
       * Listening -- grabs AMQP messages off the channel when they arrive
       * Message-wait -- any incomplete multi-part Dripline message will setup a thread to wait 
       *                 until the message is complete, and then submits it for handling
       * Receiver -- grabs completed Dripline messages and handles it
       * Async endpoint listening -- same as abovefor each asynchronous endpoint
       * Async endpoint message-wait -- same as above for each asynchronous endpoint
       * Async endpoint receiver -- same as above for each asynchronous endpoint
       * Heatbeater -- sends regular heartbeat messages
       * Scheduler -- executes scheduled events

     In addition to receiving messages from the broker, a user or client code can give messages directly to the service 
     using `process_message(message)`.
    */
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
            /* 
               \brief Extracts necessary configuration and authentication information and prepares the service to interact with the RabbitMQ broker. Does not initiate connection to the broker.
               @param a_config Dripline configuration object.  The `name` must be unique for each service.  The `dripline.broker` (and `dripline.broker_port` if needed) should be made appropriate for the mesh.  
                 The other parameters can be left as their defaults, or should be made uniform across the mesh.
                 - *Service parameters*
                   - `name` (string; default: dlcpp_service) -- Name of the service and the queue used by the service
                   - `enable_scheduling` (bool; default: false) -- Flag for enabling the scheduler
                   - `broadcast_key` (string; default: broadcast) -- Routing key used for broadcasts
                   - `loop_timeout_ms` (int; default: 1000) -- Maximum time used for listening timeouts (e.g. waiting for replies) in ms
                   - `message_wait_ms` (int; default: 1000) -- Maximum time used to wait for another AMQP message before declaring a DL message complete, in ms
                   - `heartbeat_interval_s` (int; default: 60) -- Interval between sending heartbeat messages in s
                 - *Dripline core parameters -- within the `dripline` config object*
                   - `dripline.broker` (string; default: localhost) -- Address of the RabbitMQ broker
                   - `dripline.broker_port` (int; default: 5672) -- Port used by the RabbitMQ broker
                   - `dripline.requests_exchange` (string; default: requests) -- Name of the exchange used for DL requests
                   - `dripline.alerts_exchange` (string; default: alerts) -- Name of the exchange used for DL alerts
                   - `dripline.heartbeat_routing_key` (string; default: heartbeat) -- Routing key used for sending heartbeats
                   - `dripline.max_payload_size` (int; default: DL_MAX_PAYLOAD_SIZE) -- Maximum size of payloads, in bytes
                   - `dripline.max_connection_attempts` (int; default: 10) -- Maximum number of attempts that will be made to connect to the broker
                   - `dripline.return_codes` (string or array of nodes; default: not present) -- Optional specification of additional return codes in the form of an array of nodes: `[{name: "<name>", value: <ret code>} <, ...>]`. 
                          If this is a string, it's treated as a file can be interpreted by the param system (e.g. YAML or JSON) using the previously-mentioned format
               @param a_auth Authentication object (type scarab::authentication); authentication specification should be processed, and the authentication data should include:
               @param a_make_connection Flag for whether or not to contact a broker; if true, this object operates in "dry-run" mode
             */
            service( const scarab::param_node& a_config, const scarab::authentication& a_auth, const bool a_make_connection = true );
//            service( const bool a_make_connection, const scarab::param_node& a_config = scarab::param_node(), const scarab::authentication& a_auth = scarab::authentication() );
            service( const service& ) = delete;
            service( service&& a_orig ) = default;
            virtual ~service();

            service& operator=( const service& ) = delete;
            service& operator=( service&& a_orig );

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
            /// If this returns false, the service should quit with an error
            bool start();

            /// Starts listening on the queue for receiving messages.
            /// If no queue was created, this does nothing.
            /// If this returns false, the service should quit with an error
            bool listen();

            /// Stops receiving messages and closes the connection to the broker.
            /// If no queue was created, this does nothing.
            /// If this returns false, the service should quit with an error
            bool stop();

        protected:
            virtual bool open_channels();

            virtual bool setup_queues();

            virtual bool bind_keys();

            virtual bool start_consuming();

            virtual bool stop_consuming();

            virtual bool remove_queue();

        public:
            /// Waits for AMQP messages arriving on the channel
            /// Returns false if the return is due to an error in this function; returns true otherwise (namely because it was canceled)
            virtual bool listen_on_queue();

            /// Sends a reply message
            virtual void send_reply( reply_ptr_t a_reply ) const;

            mv_accessible( uuid_t, id );

        public:
            typedef std::map< std::string, endpoint_ptr_t > sync_map_t;
            mv_referrable( sync_map_t, sync_children );

            typedef std::map< std::string, lr_ptr_t > async_map_t;
            mv_referrable( async_map_t, async_children );

            mv_referrable( std::string, broadcast_key );

        protected:
            /// Implementation of submit_message (from concurrent_receiver)
            virtual void submit_message( message_ptr_t a_message );

            virtual reply_ptr_t on_request_message( const request_ptr_t a_request );

        private:
            virtual void do_cancellation( int a_code );
    };

    inline sent_msg_pkg_ptr service::send( request_ptr_t a_request ) const
    {
        a_request->sender_service_name() = f_name;
        // we don't use f_channel on this core::send command because a channel can only be used in a single thread, 
        // and f_channel is primarily meant for listening with the listener thread.
        return core::send( a_request );
    }

    inline sent_msg_pkg_ptr service::send( reply_ptr_t a_reply ) const
    {
        a_reply->sender_service_name() = f_name ;
        // we don't use f_channel on this core::send command because a channel can only be used in a single thread, 
        // and f_channel is primarily meant for listening with the listener thread.
        return core::send( a_reply );
    }

    inline sent_msg_pkg_ptr service::send( alert_ptr_t a_alert ) const
    {
        a_alert->sender_service_name() = f_name;
        // we don't use f_channel on this core::send command because a channel can only be used in a single thread, 
        // and f_channel is primarily meant for listening with the listener thread.
        return core::send( a_alert );
    }

} /* namespace dripline */

#endif /* DRIPLINE_SERVICE_HH_ */
