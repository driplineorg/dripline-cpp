/*
 * endpoint.hh
 *
 *  Created on: Aug 14, 2018
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_ENDPOINT_HH_
#define DRIPLINE_ENDPOINT_HH_

#include "message.hh"
#include "return_codes.hh"

namespace dripline
{

    /*!
     @class endpoint
     @author N.S. Oblath

     @brief Basic Dripline object capable of receiving and acting on messages.

     @details
     This class encapsulates the basic behavior of a Dripline endpoint.  It can receive and handle messages, and 
     in particular can act on the different request message operations.

     An implementation of a particular endpoint should be a class that inherits from `endpoint`.

     An endpoint typically operates as part of a service, and it maintains a link to that service that is primarily 
     used to send messages.

     The main behaviors of the endpoint will be broken down by section:

     # Direct message submission

     Messages can be submitted directly to an endpoint using `submit_request_message()`, `submit_reply_message()`, 
     and `submit_alert_message()`.  `submit_request_message()` will return a `reply_package_ptr` to be used to 
     get the endpoint's reply.

     # Message handling

     ## Requests

     Basic request handling is performed by the `on_request_message()` function.  This can be overridden, but in most 
     cases should not need to be.  This function performs the following tasks:

     1. Checks that the request message and the lockout key it contains are valid (does not authenticate the lockout key).
     2. Passes the reqest to the `__do_[OP]_request()` function according to the request's operation type.
     3. Receives a reply object from the `__do_[OP]_request()` function
     4. If a valid reply was received (i.e. it has a reply-to address), sends the reply. Otherwise prints a message to the terminal with the results.

     Each message operation is handled in two functions: `__do_[OP]_request()` and `do_[OP]_request()`.  The former takes care 
     of built-in Dripline-standard behavior and should not be overridden.  Endpoint-specific behavior should be implemented by 
     overriding the latter.

     ### OP_GET

     * `__do_get_request()`: handles get-is-locked if relevant; otherwise calls `do_get_request().
     * `do_get_request()`: override this to add get-handling behavior.  Default sends an error reply.

     ### OP_SET

     * `__do_set_request()`: authenticates the lockout key, then calls `do_set_request().
     * `do_set_request()`: override this to add set-handling behavior.  Default sends an error reply.

     ### OP_CMD

     * `__do_cmd_request()`: authenticates the lockout key.  If relevant, handles cmd-lock, cmd-unlock, cmd-set-condition, and cmd-ping; otherwise calls `do_cmd_request()`. 
     * `do_cmd_request()`: override this to add get-handling behavior.  Default sends an error reply.

     ## Alerts

     Alert handling can be enabled by overridding the function `on_alert_message()`.  By default these are not handled.

     ## Replies

     Reply handling can be enabled by overriding the function `on_reply_message()`. By default these are not handled.

     # Lockout

     These functions implement the basic lockout functionality that's part of the Dripline standard: enabling and disabling 
     the lock, checking the lock, and validating a key.

     # Specific request handlers

     There are a few types of requests that are built into the Dripline standard, and these are handled by `endpoint`:

     * Type: `OP_CMD`; Specifier: `lock` -- set the lockout for this endpoint using the provided lockout key
     * Type: `OP_CMD`; Specifier: `unlock` --  disables the lockout for this endpoint
     * Type: `OP_GET`; Specifier: `is-locked` -- checks whether this endpoint is locked out
     * Type: `OP_CMD`; Specifier: `set-condition` -- set a particular "condition" for the endpoint.  The default behavior is to do nothing, which can be overridden with the function `__do_handle_set_condition_request()`.
     * Type: `OP_CMD`; Specifier: `ping` -- send a simple acknowledgement of receipt of the request

    */
    class DRIPLINE_API endpoint
    {
        public:
            endpoint( const std::string& a_name );
            endpoint( const endpoint& a_orig );
            endpoint( endpoint&& a_orig );
            virtual ~endpoint();

            endpoint& operator=( const endpoint& a_orig );
            endpoint& operator=( endpoint&& a_orig );

        public:
            mv_referrable( std::string, name );

            mv_referrable( service_ptr_t, service );

        public:
            //**************************
            // Direct message submission
            //**************************

            /// Directly submit a request message to this endpoint
            reply_ptr_t submit_request_message( const request_ptr_t a_request );

            /// Directly submit a reply message to this endpoint
            void submit_reply_message( const reply_ptr_t a_reply );

            /// Directly submit an alert message to this endpoint
            void submit_alert_message( const alert_ptr_t a_alert );

        public:
            //*************************
            // Default message handlers
            //*************************

            /// Default request handler; passes request to initial request functions
            virtual reply_ptr_t on_request_message( const request_ptr_t a_request );

            /// Default reply handler; throws a dripline_error.
            /// Override this to enable handling of replies.
            virtual void on_reply_message( const reply_ptr_t a_reply );

            /// Default alert handler; throws a dripline_error.
            /// Override this to enable handling of alerts.
            virtual void on_alert_message( const alert_ptr_t a_alert );

#ifdef DL_PYTHON
            mv_referrable( std::string, py_throw_reply_keyword );
#endif

        public:
            //*************************
            // Default request handlers
            //*************************

            // Override the relevant function to implement use of that type of message

            virtual reply_ptr_t do_run_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_get_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_set_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_cmd_request( const request_ptr_t a_request );

            /// Calls the appropriate request handler on a message object.
            /// Message type is explicitly checked within this function.
            /// Note that you cannot get the reply_ptr_t using this method if you submit a request.
            /// This is really intended for use when handling messages received by a parent object.
            void sort_message( const message_ptr_t a_request );

        private:
            //**************************
            // Initial request functions
            //**************************

            // Do not override
            // Authentication is checked as necessary, and then request handlers are called

            reply_ptr_t __do_run_request( const request_ptr_t a_request );
            reply_ptr_t __do_get_request( const request_ptr_t a_request );
            reply_ptr_t __do_set_request( const request_ptr_t a_request );
            reply_ptr_t __do_cmd_request( const request_ptr_t a_request );

        protected:
            virtual void send_reply( reply_ptr_t a_reply ) const;

        public:
            //******************
            // Lockout functions
            //******************

            /// enable lockout with randomly-generated key
            uuid_t enable_lockout( const scarab::param_node& a_tag );
            /// enable lockout with user-supplied key
            uuid_t enable_lockout( const scarab::param_node& a_tag, uuid_t a_key );
            bool disable_lockout( const uuid_t& a_key, bool a_force = false );
            bool is_locked() const;
            bool check_key( const uuid_t& a_key ) const;

        protected:
            /// Returns true if the server is unlocked or if it's locked and the key matches the lockout key; returns false otherwise.
            bool authenticate( const uuid_t& a_key ) const;

            mv_referrable( scarab::param_node, lockout_tag );
            mv_accessible( uuid_t, lockout_key );

        private:
            //*****************
            // Request handlers
            //*****************

            reply_ptr_t handle_lock_request( const request_ptr_t a_request );
            reply_ptr_t handle_unlock_request( const request_ptr_t a_request );
            reply_ptr_t handle_is_locked_request( const request_ptr_t a_request );
            reply_ptr_t handle_set_condition_request( const request_ptr_t a_request );
            reply_ptr_t handle_ping_request( const request_ptr_t a_request );

            /// Default set-condition: no action taken; override for different behavior
            virtual reply_ptr_t __do_handle_set_condition_request( const request_ptr_t a_request );

    };

    inline reply_ptr_t endpoint::do_run_request( const request_ptr_t a_request )
    {
        return a_request->reply( dl_device_error(), "Unhandled request type: OP_RUN" );
    }

    inline reply_ptr_t endpoint::do_get_request( const request_ptr_t a_request )
    {
        return a_request->reply( dl_device_error(), "Unhandled request type: OP_GET" );
    }

    inline reply_ptr_t endpoint::do_set_request( const request_ptr_t a_request )
    {
        return a_request->reply( dl_device_error(), "Unhandled request type: OP_SET" );
    }

    inline reply_ptr_t endpoint::do_cmd_request( const request_ptr_t a_request )
    {
        return a_request->reply( dl_device_error(), "Unhandled request type: OP_CMD" );
    }

    inline uuid_t endpoint::enable_lockout( const scarab::param_node& a_tag )
    {
        return enable_lockout( a_tag, generate_random_uuid() );
    }

    inline bool endpoint::is_locked() const
    {
        return ! f_lockout_key.is_nil();
    }

    inline bool endpoint::check_key( const uuid_t& a_key ) const
    {
        return f_lockout_key == a_key;
    }

    inline reply_ptr_t endpoint::__do_handle_set_condition_request( const request_ptr_t a_request )
    {
        return a_request->reply( dl_success(), "No action taken (this is the default method)" );
    }

} /* namespace dripline */

#endif /* DRIPLINE_ENDPOINT_HH_ */
