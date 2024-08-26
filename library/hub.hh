/*
 * hub.hh
 *
 *  Created on: Jan 7, 2016
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_HUB_HH_
#define DRIPLINE_HUB_HH_

#include "service.hh"

#include "dripline_exceptions.hh"

#include <unordered_map>

namespace dripline
{

    /*!
     @class hub
     @author N.S. Oblath

     @brief Service class aimed at adding a Dripline API to an existing codebase.

     @details
     Hub is a tool to a Dripline API onto existing codebase.  Message-handler functions 
     in the codebase are mapped to Dripline run, command, get, and set requests.

     The handler functions need to have the signature `reply_ptr_t( const request_ptr_t )`.  
     Typically those handler functions will wrap another function in the codebase and provide the 
     tranlation between dripline message and the input/output of the function.

     The key for the request-->function mapping for any given request message are given in the 
     message's specifier.

     As an example of adding a Dripline API, say you had class `foo`, which had a function `int bar( int )`:
     ~~~
     class foo
     {
         int bar( int a_input )
         {
             return 2 * a_input;
         }
     };
     ~~~

     To add a Dripline API for this class you first need a message-handling function:
     ~~~
     class foo
     {
         [...]
         dripline::reply_ptr_t handle_bar( dripline::request_ptr_t a_request )
         {
             param_ptr_t t_reply_payload( new param_node() );
             t_reply_payload->as_node().add( "value", bar( a_request->payload()["values"][0]().as_int() );
             return a_request->reply( dl_success(), "Bar request succeeded", std::move(t_reply_payload) );
         }
     };
     ~~~

     Then you would create a hub as part of the codebase, and either in the hub or somewhere else 
     you would register the bar handler function:
     ~~~
     register_cmd_handler( "bar", std::bind( &foo::handle_bar, this, _1 ) );
     ~~~
     In this case we choose to make it a Dripline command request, since that seems to fit the best 
     with performing the foo::bar operation.
    */
    class DRIPLINE_API hub : public service
    {
        private:
            typedef std::function< reply_ptr_t( const dripline::request_ptr_t ) > handler_func_t;

        public:
            /* 
               \brief Extracts necessary configuration and authentication information and prepares the hub to interact with the RabbitMQ broker. Does not initiate connection to the broker.
               @param a_config Dripline configuration object.  The `name` must be unique for each hub.  The `dripline.broker` (and `dripline.broker-port` if needed) should be made appropriate for the mesh.  
                 The other parameters can be left as their defaults, or should be made uniform across the mesh.
                 - *Service parameters*
                   - `name` (string; default: dlcpp_service) -- Name of the service and of the queue used by the service
                   - `enable-scheduling` (bool; default: false) -- Flag for enabling the scheduler
                   - `broadcast-key` (string; default: broadcast) -- Routing key used for broadcasts
                   - `loop-timeout-ms` (int; default: 1000) -- Maximum time used for listening timeouts (e.g. waiting for replies) in ms
                   - `message-wait-ms` (int; default: 1000) -- Maximum time used to wait for another AMQP message before declaring a DL message complete, in ms
                   - `heartbeat-interval-s` (int; default: 60) -- Interval between sending heartbeat messages in s
                 - *Dripline core parameters -- within the `dripline` config object*
                   - `dripline.broker` (string; default: localhost) -- Address of the RabbitMQ broker
                   - `dripline.broker-port` (int; default: 5672) -- Port used by the RabbitMQ broker
                   - `dripline.requests-exchange` (string; default: requests) -- Name of the exchange used for DL requests
                   - `dripline.alerts-exchange` (string; default: alerts) -- Name of the exchange used for DL alerts
                   - `dripline.heartbeat-routing-key` (string; default: heartbeat) -- Routing key used for sending heartbeats
                   - `dripline.max-payload-size` (int; default: DL_MAX_PAYLOAD_SIZE) -- Maximum size of payloads, in bytes
                   - `dripline.max-connection-attempts` (int; default: 10) -- Maximum number of attempts that will be made to connect to the broker
                   - `dripline.return-codes` (string or array of nodes; default: not present) -- Optional specification of additional return codes in the form of an array of nodes: `[{name: "<name>", value: <ret code>} <, ...>]`. 
                          If this is a string, it's treated as a file can be interpreted by the param system (e.g. YAML or JSON) using the previously-mentioned format
               @param a_auth Authentication object (type scarab::authentication); authentication specification should be processed, and the authentication data should include:
               @param a_make_connection Flag for whether or not to contact a broker; if true, this object operates in "dry-run" mode
             */
            hub( const scarab::param_node& a_config, const scarab::authentication& a_auth, const bool a_make_connection = true );
            hub( const hub& ) = delete;
            hub( hub&& ) = default;
            virtual ~hub() = default;

            hub& operator=( const hub& ) = delete;
            hub& operator=( hub&& );

            /// Sets the run request handler function
            void set_run_handler( const handler_func_t& a_func );
            /// Sets a get request handler function
            void register_get_handler( const std::string& a_key, const handler_func_t& a_func );
            /// Sets a set request handler function
            void register_set_handler( const std::string& a_key, const handler_func_t& a_func );
            /// Sets a command request handler function
            void register_cmd_handler( const std::string& a_key, const handler_func_t& a_func );

            /// Removes a get request handler function
            void remove_get_handler( const std::string& a_key );
            /// Removes a set request handler function
            void remove_set_handler( const std::string& a_key );
            /// Removes a command request handler function
            void remove_cmd_handler( const std::string& a_key );

        private:
            //*************************
            // Hub request distributors
            //*************************

            virtual reply_ptr_t do_run_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_get_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_set_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_cmd_request( const request_ptr_t a_request );

            handler_func_t f_run_handler;

            typedef std::unordered_map< std::string, handler_func_t > handler_funcs_t;
            handler_funcs_t f_get_handlers;
            handler_funcs_t f_set_handlers;
            handler_funcs_t f_cmd_handlers;

    };

} /* namespace dripline */

#endif /* DRIPLINE_HUB_HH_ */
