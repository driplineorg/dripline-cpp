/*
 * return_codes.cc
 *
 *  Created on: Aug 25, 2018
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "return_codes.hh"

#include "logger.hh"

LOGGER( rclog, "return_codes" );

namespace dripline
{
    copy_code::copy_code( const return_code& a_code ) :
            f_value( a_code.rc_value() ),
            f_name( a_code.rc_name() ),
            f_description( a_code.rc_description() )
    {}

    //****************
    // Return code implementations
    //****************

    IMPLEMENT_DL_RET_CODE( success, 0, "Success" );

    IMPLEMENT_DL_RET_CODE( warning_no_action_taken, 1, "No Action Taken" );

    IMPLEMENT_DL_RET_CODE( amqp_error, 100, "AMQP Error" );
    IMPLEMENT_DL_RET_CODE( amqp_error_broker_connection, 101, "AMQP Connection Error" );
    IMPLEMENT_DL_RET_CODE( amqp_error_routingkey_notfound, 102, "AMQP Routing Key Error" );

    IMPLEMENT_DL_RET_CODE( device_error, 200, "Device Error" );
    IMPLEMENT_DL_RET_CODE( device_error_connection, 201, "Connection Error" );
    IMPLEMENT_DL_RET_CODE( device_error_no_resp, 202, "No Response Error" );

    IMPLEMENT_DL_RET_CODE( message_error, 300, "Dripline Message Error" );
    IMPLEMENT_DL_RET_CODE( message_error_no_encoding, 301, "No Message Encoding" );
    IMPLEMENT_DL_RET_CODE( message_error_decoding_fail, 302, "Decoding Failed" );
    IMPLEMENT_DL_RET_CODE( message_error_bad_payload, 303, "Bad Payload" );
    IMPLEMENT_DL_RET_CODE( message_error_invalid_value, 304, "Invalid Value" );
    IMPLEMENT_DL_RET_CODE( message_error_timeout, 305, "Timeout Handling Message" );
    IMPLEMENT_DL_RET_CODE( message_error_invalid_method, 306, "Invalid Method" );
    IMPLEMENT_DL_RET_CODE( message_error_access_denied, 307, "Access Denied" );
    IMPLEMENT_DL_RET_CODE( message_error_invalid_key, 308, "Invalid Key" );
    IMPLEMENT_DL_RET_CODE( message_error_dripline_deprecated, 309, "Deprecated Feature" );
    IMPLEMENT_DL_RET_CODE( message_error_invalid_specifier, 310, "Invalid Specifier" );

    IMPLEMENT_DL_RET_CODE( client_error, 400, "Client Error" );
    IMPLEMENT_DL_RET_CODE( client_error_invalid_request, 401, "Invalid Request" );
    IMPLEMENT_DL_RET_CODE( client_error_handling_reply, 402, "Error Handling Reply" );
    IMPLEMENT_DL_RET_CODE( client_error_unable_to_send, 403, "Unable to Send" );
    IMPLEMENT_DL_RET_CODE( client_error_timeout, 404, "Client Timeout" );

    IMPLEMENT_DL_RET_CODE( unhandled_exception, 999, "Unhandled Exception" );

} /* namespace dripline */
