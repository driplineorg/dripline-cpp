/*
 * constants.hh
 *
 *  Created on: Jan 5, 2016
 *      Author: nsoblath
 */

#ifndef DRIPLINE_CONSTANTS_HH_
#define DRIPLINE_CONSTANTS_HH_

#include "dripline_api.hh"

#include <cstdint>
#include <limits>
#include <ostream>

namespace dripline
{

    // Return value constants
#define RETURN_SUCCESS 1
#define RETURN_ERROR -1
#define RETURN_CANCELED -2
#define RETURN_REVOKED -3


    // Dripline message constants
    // Conforming to the dripline wire protocol: https://github.com/project8/hardware/wiki/Wire-Protocol
    // Please be sure that these constants are kept in sync with the dripline constants.

    // Operation constants
    enum class op_t:uint32_t {
            set = 0,
            get = 1,
            config = 6,
            send = 7,
            run = 8,
            cmd = 9,
            unknown = UINT32_MAX
    };

    // Conversion functions for use when a numeric value is needed
    DRIPLINE_API uint32_t to_uint( op_t an_op );
    DRIPLINE_API op_t to_op_t( uint32_t an_op_uint );
    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, op_t an_op );

    // Message type constants
    enum class msg_t:uint32_t
    {
        reply = 2,
        request = 3,
        alert = 4,
        info = 5
    };

    // Conversion functions for use when a numeric value is needed
    DRIPLINE_API uint32_t to_uint( msg_t a_msg );
    DRIPLINE_API msg_t to_msg_t( uint32_t a_msg_uint );
    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, msg_t a_msg );

    // Return codes
    enum class retcode_t:uint32_t
    {
        success = 0,

        warning_no_action_taken = 1,

        amqp_error = 100,
        amqp_error_broker_connection = 101,
        amqp_error_routingkey_notfound = 102,

        device_error = 200,
        device_error_connection = 201,
        device_error_no_resp = 202,

        message_error = 300,
        message_error_no_encoding = 301,
        message_error_decoding_fail = 302,
        message_error_bad_payload = 303,
        message_error_invalid_value = 304,
        message_error_timeout = 305,
        message_error_invalid_method = 306,
        message_error_access_denied = 307,
        message_error_invalid_key = 308,
        message_error_dripline_deprecated = 309
    };

    // Conversion functions for use when a numeric value is needed
    DRIPLINE_API uint32_t to_uint( retcode_t a_ret );
    DRIPLINE_API retcode_t to_retcode_t( uint32_t a_ret_uint );
    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, retcode_t a_ret );

} /* namespace dripline */

#endif /* DRIPLINE_CONSTANTS_HH_ */
