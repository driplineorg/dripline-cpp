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
    //****************
    // Return code implementations
    //****************

    IMPLEMENT_DL_RET_CODE( success, 0 );

    IMPLEMENT_DL_RET_CODE( warning_no_action_taken, 1 );

    IMPLEMENT_DL_RET_CODE( amqp_error, 100 );
    IMPLEMENT_DL_RET_CODE( amqp_error_broker_connection, 101 );
    IMPLEMENT_DL_RET_CODE( amqp_error_routingkey_notfound, 102 );

    IMPLEMENT_DL_RET_CODE( device_error, 200 );
    IMPLEMENT_DL_RET_CODE( device_error_connection, 201 );
    IMPLEMENT_DL_RET_CODE( device_error_no_resp, 202 );

    IMPLEMENT_DL_RET_CODE( message_error, 300 );
    IMPLEMENT_DL_RET_CODE( message_error_no_encoding, 301 );
    IMPLEMENT_DL_RET_CODE( message_error_decoding_fail, 302 );
    IMPLEMENT_DL_RET_CODE( message_error_bad_payload, 303 );
    IMPLEMENT_DL_RET_CODE( message_error_invalid_value, 304 );
    IMPLEMENT_DL_RET_CODE( message_error_timeout, 305 );
    IMPLEMENT_DL_RET_CODE( message_error_invalid_method, 306 );
    IMPLEMENT_DL_RET_CODE( message_error_access_denied, 307 );
    IMPLEMENT_DL_RET_CODE( message_error_invalid_key, 308 );
    IMPLEMENT_DL_RET_CODE( message_error_dripline_deprecated, 309 );
    IMPLEMENT_DL_RET_CODE( message_error_invalid_specifier, 310 );

    IMPLEMENT_DL_RET_CODE( client_error, 400 );
    IMPLEMENT_DL_RET_CODE( client_error_invalid_request, 401 );
    IMPLEMENT_DL_RET_CODE( client_error_handling_reply, 402 );
    IMPLEMENT_DL_RET_CODE( client_error_unable_to_send, 403 );
    IMPLEMENT_DL_RET_CODE( client_error_timeout, 404 );

    IMPLEMENT_DL_RET_CODE( unhandled_exception, 999 );

} /* namespace dripline */
