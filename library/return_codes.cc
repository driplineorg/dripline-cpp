/*
 * return_codes.cc
 *
 *  Created on: Aug 25, 2018
 *      Author: N.S. Oblath
 */

#include "return_codes.hh"

namespace dripline
{
    IMPL_DL_RET_CODE( success, 0 );

    IMPL_DL_RET_CODE( warning_no_action_taken, 1 );

    IMPL_DL_RET_CODE( amqp_error, 100 );
    IMPL_DL_RET_CODE( amqp_error_broker_connection, 101 );
    IMPL_DL_RET_CODE( amqp_error_routingkey_notfound, 102 );

    IMPL_DL_RET_CODE( device_error, 200 );
    IMPL_DL_RET_CODE( device_error_connection, 201 );
    IMPL_DL_RET_CODE( device_error_no_resp, 202 );

    IMPL_DL_RET_CODE( message_error, 300 );
    IMPL_DL_RET_CODE( message_error_no_encoding, 301 );
    IMPL_DL_RET_CODE( message_error_decoding_fail, 302 );
    IMPL_DL_RET_CODE( message_error_bad_payload, 303 );
    IMPL_DL_RET_CODE( message_error_invalid_value, 304 );
    IMPL_DL_RET_CODE( message_error_timeout, 305 );
    IMPL_DL_RET_CODE( message_error_invalid_method, 306 );
    IMPL_DL_RET_CODE( message_error_access_denied, 307 );
    IMPL_DL_RET_CODE( message_error_invalid_key, 308 );
    IMPL_DL_RET_CODE( message_error_dripline_deprecated, 309 );

    IMPL_DL_RET_CODE( database_error, 400 );

    IMPL_DL_RET_CODE( daq_error, 500 );
    IMPL_DL_RET_CODE( daq_not_enabled, 501 );
    IMPL_DL_RET_CODE( daq_running, 502 );

    IMPL_DL_RET_CODE( unhandled_exception, 999 );


    return_code::return_code() :
            f_message()
    {
    }

    return_code::return_code( const return_code& a_orig ) :
            f_message( a_orig.f_message )
    {
    }

    return_code::return_code( return_code&& a_orig ) :
            f_message( std::move(a_orig.f_message) )
    {
    }

    return_code::~return_code()
    {
    }

    return_code& return_code::operator=( const return_code& a_orig )
    {
        f_message = a_orig.f_message;
        return *this;
    }

    return_code& return_code::operator=( return_code&& a_orig )
    {
        f_message = std::move(a_orig.f_message);
        return *this;
    }

} /* namespace dripline */
