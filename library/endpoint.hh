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
    class endpoint;
    typedef std::shared_ptr< endpoint > endpoint_ptr_t;

    class service;

    class DRIPLINE_API endpoint
    {
        public:
            endpoint( const std::string& a_name, service& a_service );
            virtual ~endpoint();

        public:
            mv_referrable_const( std::string, name );

        protected:
            friend class service;
            service& f_service;

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

        protected:
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

        protected:
            //*************************
            // Default request handlers
            //*************************

            // Override the relevant function to implement use of that type of message

            virtual reply_ptr_t do_run_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_get_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_set_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_cmd_request( const request_ptr_t a_request );

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
            void send_reply( reply_ptr_t a_reply ) const;

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
