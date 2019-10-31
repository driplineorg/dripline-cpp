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
            hub( const scarab::param_node& a_config = scarab::param_node(), const std::string& a_queue_name = "",  const std::string& a_broker_address = "", unsigned a_port = 0, const std::string& a_auth_file = "" , const bool a_make_connection = true );
            virtual ~hub();

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
