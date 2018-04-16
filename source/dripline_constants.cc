/*
 * constants.cc
 *
 *  Created on: Jan 5, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "dripline_constants.hh"
#include "dripline_error.hh"

namespace dripline
{

    // Conversion functions for use when a numeric value is needed
    DRIPLINE_API uint32_t to_uint( op_t an_op )
    {
        return static_cast< uint32_t >( an_op );
    }
    DRIPLINE_API op_t to_op_t( uint32_t an_op_uint )
    {
        return static_cast< op_t >( an_op_uint );
    }
    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, op_t an_op )
    {
        return a_os << to_uint( an_op );
    }
    // Conversion functions for use when string values are required
    // TODO op_t->string isn't too bad, but the string->op_t seems super ugly
    DRIPLINE_API std::string to_string( op_t an_op )
    {
        switch (an_op) {
            case op_t::set: return "set";
            case op_t::get: return "get";
            case op_t::config: return "config";//config is deprecated
            case op_t::send: return "send";
            case op_t::run: return "run";
            case op_t::cmd: return "cmd";
            case op_t::unknown: return "unknown";
            default: throw dripline_error() << "op_t value <" << an_op << "> not recognized";
        }
        //TODO explicitly throw something here?
    }
    DRIPLINE_API op_t to_op_t( std::string an_op_str )
    {
        if ( an_op_str == to_string( op_t::set ) ) return op_t::set;
        if ( an_op_str == to_string( op_t::get ) ) return op_t::get;
        if ( an_op_str == to_string( op_t::config ) ) return op_t::config;
        if ( an_op_str == to_string( op_t::send ) ) return op_t::send;
        if ( an_op_str == to_string( op_t::run ) ) return op_t::run;
        if ( an_op_str == to_string( op_t::cmd ) ) return op_t::cmd;
        if ( an_op_str == to_string( op_t::unknown ) ) return op_t::unknown;
        throw dripline_error() << "unable to map <" << an_op_str << "> to an op_t value";
        //TODO explicitly throw something here?
    }

    // Conversion functions for use when a numeric value is needed
    DRIPLINE_API uint32_t to_uint( msg_t a_msg )
    {
        return static_cast< uint32_t >( a_msg );
    }
    DRIPLINE_API msg_t to_msg_t( uint32_t a_msg_uint )
    {
        return static_cast< msg_t >( a_msg_uint );
    }
    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, msg_t a_msg )
    {
        return a_os << to_uint( a_msg );
    }

    // Conversion functions for use when a numeric value is needed
    DRIPLINE_API uint32_t to_uint( retcode_t a_ret )
    {
        return static_cast< uint32_t >( a_ret );
    }
    DRIPLINE_API retcode_t to_retcode_t( uint32_t a_ret_uint )
    {
        return static_cast< retcode_t >( a_ret_uint );
    }
    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, retcode_t a_ret )
    {
        return a_os << to_uint( a_ret );
    }

} /* namespace dripline */
