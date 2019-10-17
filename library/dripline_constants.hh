/*
 * dripline_constants.hh
 *
 *  Created on: Jan 5, 2016
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_CONSTANTS_HH_
#define DRIPLINE_CONSTANTS_HH_

#include "dripline_api.hh"

#include "macros.hh"

#include <cstdint>
#include <limits>
#include <ostream>

namespace dripline
{
#ifndef DL_MAX_PAYLOAD_SIZE
#define DL_MAX_PAYLOAD_SIZE 10000
#endif

    // Dripline message constants
    // Conforming to the dripline wire protocol: https://github.com/project8/hardware/wiki/Wire-Protocol
    // Please be sure that these constants are kept in sync with the dripline constants.

    /*!
        \enum op_t
        Message Operations
    */
    enum class DRIPLINE_API op_t:uint32_t {
            set = 0,
            get = 1,
            config = 6, // deprecated as of v2.0.0
            send = 7,
            run = 8,
            cmd = 9,
            unknown = UINT32_MAX
    };

    /// Convert a message-operation enum to an integer
    DRIPLINE_API uint32_t to_uint( op_t an_op );
    /// Convert an integer to a message-operation enum
    /// The result is unspecified for invalid integers
    DRIPLINE_API op_t to_op_t( uint32_t an_op_uint );
    /// Pass the integer-equivalent of a message-operation enum to an ostream
    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, op_t an_op );
    /// Gives the human-readable version of a message operation
    DRIPLINE_API std::string to_string( op_t an_op );
    /// Gives the message-operation enum for a string
    /// Throws dripline_error for invalid strings
    DRIPLINE_API op_t to_op_t( std::string an_op_str );

    /*!
        \enum msg_t
        Message Types
    */
    enum class DRIPLINE_API msg_t:uint32_t
    {
        reply = 2,
        request = 3,
        alert = 4,
        unknown = UINT32_MAX
    };

    /// Convert a message-type enum to an integer
    DRIPLINE_API uint32_t to_uint( msg_t a_msg );
    /// Convert an integer to a message-type enum
    /// The result is unspecified for invalid integers
    DRIPLINE_API msg_t to_msg_t( uint32_t a_msg_uint );
    /// Pass the integer-equivalent of a message-type enum to an ostream
    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, msg_t a_msg );
    /// Gives the human-readable version of the message type
    DRIPLINE_API std::string to_string( msg_t a_msg );
    /// Gives the message-type enum for a string
    /// Throws dripline_error for invalid strings
    DRIPLINE_API msg_t to_msg_t( std::string a_msg_str );

} /* namespace dripline */

#endif /* DRIPLINE_CONSTANTS_HH_ */
