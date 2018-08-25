/*
 * return_codes.cc
 *
 *  Created on: Aug 25, 2018
 *      Author: N.S. Oblath
 */

#include "return_codes.hh"

namespace dripline
{
    unsigned return_success::f_code = 0;

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
