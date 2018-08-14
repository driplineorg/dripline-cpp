/*
 * endpoint.cc
 *
 *  Created on: Aug 14, 2018
 *      Author: N.S. Oblath
 */

#include "endpoint.hh"

#include "dripline_error.hh"

namespace dripline
{

    endpoint::endpoint( const std::string& a_name, service& a_service ) :
            f_name( a_name ),
            f_service( a_service )
    {
    }

    endpoint::~endpoint()
    {
    }

    reply_info endpoint::submit_request_message( const request_ptr_t a_request_ptr)
    {
        return this->on_request_message( a_request_ptr );;
    }

    bool endpoint::submit_alert_message( const alert_ptr_t a_alert_ptr)
    {
        return this->on_alert_message( a_alert_ptr );
    }

    bool endpoint::submit_reply_message( const reply_ptr_t a_reply_ptr)
    {
        return this->on_reply_message( a_reply_ptr );
    }

    reply_info endpoint::on_request_message( const request_ptr_t )
    {
        throw dripline_error() << retcode_t::message_error_invalid_method << "Base service does not handle request messages";
    }

    bool endpoint::on_reply_message( const reply_ptr_t )
    {
        throw dripline_error() << retcode_t::message_error_invalid_method << "Base service does not handle reply messages";
    }

    bool endpoint::on_alert_message( const alert_ptr_t )
    {
        throw dripline_error() << retcode_t::message_error_invalid_method << "Base service does not handle alert messages";
    }




} /* namespace dripline */
