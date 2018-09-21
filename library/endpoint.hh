/*
 * endpoint.hh
 *
 *  Created on: Aug 14, 2018
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_ENDPOINT_HH_
#define DRIPLINE_ENDPOINT_HH_

#include "message.hh"
#include "reply_package.hh"
#include "return_codes.hh"

namespace dripline
{
    class service;

    class DRIPLINE_API endpoint
    {
        public:
            endpoint( const std::string& a_name, service& a_service );
            virtual ~endpoint();

        public:
            mv_referrable_const( std::string, name );

        protected:
            service& f_service;

        public:
            //**************************
            // Direct message submission
            //**************************

            /// Directly submit a request message to this endpoint
            void submit_request_message( const request_ptr_t a_request );

            /// Directly submit a reply message to this endpoint
            void submit_reply_message( const reply_ptr_t a_reply );

            /// Directly submit an alert message to this endpoint
            void submit_alert_message( const alert_ptr_t a_alert );

        protected:
            //*************************
            // Default message handlers
            //*************************

            /// Default request handler; passes request to initial request functions
            virtual void on_request_message( const request_ptr_t a_request );

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

        public:
            //******************
            // Lockout functions
            //******************

            bool is_locked() const;
            const scarab::param_node& get_lockout_tag() const;
            bool check_key( const uuid_t& a_key ) const;

        private:
            //*****************
            // Request handlers
            //*****************

            reply_ptr_t handle_ping_request( const request_ptr_t a_request );

        protected:
            void send_reply( reply_ptr_t a_reply ) const;
    };

    inline reply_ptr_t endpoint::do_run_request( const request_ptr_t a_request )
    {
        return a_request->template reply< dl_device_error >( "Unhandled request type: OP_RUN" );
    }

    inline reply_ptr_t endpoint::do_get_request( const request_ptr_t a_request )
    {
        return a_request->template reply< dl_device_error >( "Unhandled request type: OP_GET" );
    }

    inline reply_ptr_t endpoint::do_set_request( const request_ptr_t a_request )
    {
        return a_request->template reply< dl_device_error >( "Unhandled request type: OP_SET" );
    }

    inline reply_ptr_t endpoint::do_cmd_request( const request_ptr_t a_request )
    {
        return a_request->template reply< dl_device_error >( "Unhandled request type: OP_CMD" );
    }

} /* namespace dripline */

#endif /* DRIPLINE_ENDPOINT_HH_ */
