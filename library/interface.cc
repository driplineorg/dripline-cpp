/*
 * interface.cc
 *
 *  Created on: Aug 15, 2019
 *      Author: N.S. Oblath
 */

#include "interface.hh"

namespace dripline
{

    interface::interface( const scarab::param_node& a_config ) :
            core( a_config )
    {}

    interface::~interface()
    {}

    sent_msg_pkg_ptr interface::get( const std::string& a_rk, const std::string& a_specifier )
    {
        request_ptr_t t_request = msg_request::create( scarab::param_ptr_t(new scarab::param()), op_t::get, a_rk, a_specifier );
        return send( t_request );
    }

    sent_msg_pkg_ptr interface::set( const std::string& a_rk, scarab::param_value a_value, const std::string& a_specifier, const std::string& a_lockout_key )
    {

    }

    sent_msg_pkg_ptr interface::cmd( const std::string& a_rk, const std::string& a_specifier, const std::string& a_lockout_key )
    {

    }


} /* namespace dripline */
