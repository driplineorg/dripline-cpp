/*
 * constants.hh
 *
 *  Created on: Jan 5, 2016
 *      Author: nsoblath
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

    // Operation constants
    enum class DRIPLINE_API op_t:uint32_t {
            set = 0,
            get = 1,
            run = 8,
            cmd = 9,
            unknown = UINT32_MAX
    };

    // Conversion functions for use when a numeric value is needed
    DRIPLINE_API uint32_t to_uint( op_t an_op );
    DRIPLINE_API op_t to_op_t( uint32_t an_op_uint );
    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, op_t an_op );
    // Conversion functions for string values
    DRIPLINE_API std::string to_string( op_t an_op );
    DRIPLINE_API op_t to_op_t( std::string an_op_str );

    // Message type constants
    enum class DRIPLINE_API msg_t:uint32_t
    {
        reply = 2,
        request = 3,
        alert = 4,
        unknown = UINT32_MAX
    };

    // Conversion functions for use when a numeric value is needed
    DRIPLINE_API uint32_t to_uint( msg_t a_msg );
    DRIPLINE_API msg_t to_msg_t( uint32_t a_msg_uint );
    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, msg_t a_msg );
    // Conversion functions for string values
    DRIPLINE_API std::string to_string( msg_t a_msg );
    DRIPLINE_API msg_t to_msg_t( std::string a_msg_str );

} /* namespace dripline */

#endif /* DRIPLINE_CONSTANTS_HH_ */
