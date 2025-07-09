/*
 * dripline_constants.cc
 *
 *  Created on: Jan 5, 2016
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "dripline_constants.hh"
#include "dripline_exceptions.hh"

namespace dripline
{
    // op_t utility functions
    // Conversion functions for use when a numeric value is needed
    DRIPLINE_API unsigned to_uint( op_t an_op )
    {
        return static_cast< unsigned >( an_op );
    }
    DRIPLINE_API op_t to_op_t( unsigned an_op_uint )
    {
        return static_cast< op_t >( an_op_uint );
    }
    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, op_t an_op )
    {
        return a_os << to_uint( an_op );
    }
    // Conversion functions for use when string values are required
    DRIPLINE_API std::string to_string( op_t an_op )
    {
        switch (an_op) {
            case op_t::set: return "set";
            case op_t::get: return "get";
            case op_t::cmd: return "cmd";
            case op_t::unknown: return "unknown";
            default: throw dripline_error() << "op_t value <" << an_op << "> not recognized";
        }
    }
    DRIPLINE_API op_t to_op_t( std::string an_op_str )
    {
        if ( an_op_str == to_string( op_t::set ) ) return op_t::set;
        if ( an_op_str == to_string( op_t::get ) ) return op_t::get;
        if ( an_op_str == to_string( op_t::cmd ) ) return op_t::cmd;
        if ( an_op_str == to_string( op_t::unknown ) ) return op_t::unknown;
        throw dripline_error() << "unable to map <" << an_op_str << "> to an op_t value";
    }

    // msg_t utility functions
    // Conversion functions for use when a numeric value is needed
    DRIPLINE_API unsigned to_uint( msg_t a_msg )
    {
        return static_cast< unsigned >( a_msg );
    }
    DRIPLINE_API msg_t to_msg_t( unsigned a_msg_uint )
    {
        return static_cast< msg_t >( a_msg_uint );
    }
    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, msg_t a_msg )
    {
        return a_os << to_uint( a_msg );
    }
    // Conversion functions for use when string values are required
    DRIPLINE_API std::string to_string( msg_t a_msg )
    {
        switch (a_msg) {
            case msg_t::reply: return "reply";
            case msg_t::request: return "request";
            case msg_t::alert: return "alert";
            case msg_t::unknown: return "unknown";
            default: throw dripline_error() << "msg_t value <" << a_msg << "> not recognized";
        }
    }
    DRIPLINE_API msg_t to_msg_t( std::string a_msg_str )
    {
        if ( a_msg_str == to_string( msg_t::reply ) ) return msg_t::reply;
        if ( a_msg_str == to_string( msg_t::request ) ) return msg_t::request;
        if ( a_msg_str == to_string( msg_t::alert ) ) return msg_t::alert;
        if ( a_msg_str == to_string( msg_t::unknown ) ) return msg_t::unknown;
        throw dripline_error() << "unable to map <" << a_msg_str << "> to a msg_t value";
    }

} /* namespace dripline */
