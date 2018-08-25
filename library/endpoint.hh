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
            reply_info submit_request_message( const request_ptr_t a_request );

            /// Directly submit a reply message to this endpoint
            bool submit_reply_message( const reply_ptr_t a_reply );

            /// Directly submit an alert message to this endpoint
            bool submit_alert_message( const alert_ptr_t a_alert );

        protected:
            //*************************
            // Default message handlers
            //*************************

            /// Default request handler; passes request to initial request functions
            virtual reply_info on_request_message( const request_ptr_t a_request );

            /// Default reply handler; throws a dripline_error.
            /// Override this to enable handling of replies.
            virtual bool on_reply_message( const reply_ptr_t a_reply );

            /// Default alert handler; throws a dripline_error.
            /// Override this to enable handling of alerts.
            virtual bool on_alert_message( const alert_ptr_t a_alert );

        protected:
            //*************************
            // Default request handlers
            //*************************

            // Override the relevant function to implement use of that type of message

            virtual reply_info do_run_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            virtual reply_info do_get_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            virtual reply_info do_set_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            virtual reply_info do_cmd_request( const request_ptr_t a_request, reply_package& a_reply_pkg );

        private:
            //**************************
            // Initial request functions
            //**************************

            // Do not override
            // Authentication is checked as necessary, and then request handlers are called

            reply_info __do_run_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            reply_info __do_get_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            reply_info __do_set_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            reply_info __do_cmd_request( const request_ptr_t a_request, reply_package& a_reply_pkg );

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

            reply_info handle_ping_request( const request_ptr_t a_request, reply_package& a_reply_pkg );

    };

    inline reply_info endpoint::do_run_request( const request_ptr_t, reply_package& a_reply_pkg )
    {
        return a_reply_pkg.send_reply( retcode_t::device_error, "Unhandled request type: OP_RUN" );;
    }

    inline reply_info endpoint::do_get_request( const request_ptr_t, reply_package& a_reply_pkg )
    {
        return a_reply_pkg.send_reply( retcode_t::device_error, "Unhandled request type: OP_GET" );;
    }

    inline reply_info endpoint::do_set_request( const request_ptr_t, reply_package& a_reply_pkg )
    {
        return a_reply_pkg.send_reply( retcode_t::device_error, "Unhandled request type: OP_SET" );;
    }

    inline reply_info endpoint::do_cmd_request( const request_ptr_t, reply_package& a_reply_pkg )
    {
        return a_reply_pkg.send_reply( retcode_t::device_error, "Unhandled request type: OP_CMD" );;
    }

} /* namespace dripline */

#endif /* DRIPLINE_ENDPOINT_HH_ */
