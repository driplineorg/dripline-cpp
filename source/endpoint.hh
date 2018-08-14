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

    class DRIPLINE_API endpoint
    {
        public:
            endpoint( const std::string& a_name, service& a_service );
            virtual ~endpoint();

        public:
            /// Directly submit a request message to this endpoint
            reply_info submit_request_message( const request_ptr_t a_request );

            /// Directly submit a reply message to this endpoint
            bool submit_reply_message( const reply_ptr_t a_reply );

            /// Directly submit an alert message to this endpoint
            bool submit_alert_message( const alert_ptr_t a_alert );

        protected:
            /// Default request handler; throws a dripline_error.
            /// Override this to enable handling of requests.
            virtual reply_info on_request_message( const request_ptr_t a_request );

            /// Default reply handler; throws a dripline_error.
            /// Override this to enable handling of replies.
            virtual bool on_reply_message( const reply_ptr_t a_reply );

            /// Default alert handler; throws a dripline_error.
            /// Override this to enable handling of alerts.
            virtual bool on_alert_message( const alert_ptr_t a_alert );

        public:
            mv_referrable_const( std::string, name );

        protected:
            service& f_service;

    };

} /* namespace dripline */

#endif /* DRIPLINE_ENDPOINT_HH_ */
