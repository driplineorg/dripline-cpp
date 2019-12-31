/*
 * dripline_exceptions.cc
 *
 *  Created on: Aug 14, 2018
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "dripline_exceptions.hh"

namespace dripline
{

    dripline_error::dripline_error() noexcept :
            base_exception< dripline_error >()
    {}

    dripline_error::dripline_error( const dripline_error& an_error ) noexcept :
            base_exception< dripline_error >( an_error )
    {}

    dripline_error::~dripline_error() noexcept
    {}


    throw_reply::throw_reply() :
            base_exception< throw_reply >(),
            f_return_code( new dl_unhandled_exception() ),
            f_payload( new scarab::param() )
    {}

    throw_reply::throw_reply( const return_code& a_code, scarab::param_ptr_t&& a_payload_ptr ) :
            base_exception< throw_reply >(),
            f_return_code( new copy_code( a_code ) ),
            f_payload( std::move(a_payload_ptr) )
    {}

    throw_reply::throw_reply( const throw_reply& a_orig ) :
            base_exception< throw_reply >( a_orig ),
            f_return_code( a_orig.f_return_code ),
            f_payload( a_orig.f_payload->clone() )
    {}

    throw_reply::~throw_reply() noexcept
    {}

    throw_reply& throw_reply::operator=( const throw_reply& a_orig )
    {
        base_exception< throw_reply >::operator=( a_orig );
        f_return_code = a_orig.f_return_code;
        f_payload = a_orig.f_payload->clone();
        return *this;
    }

    const char* throw_reply::what() const noexcept
    {
        std::stringstream t_stream;
        t_stream << f_error;
        f_error = t_stream.str();
        return base_exception< throw_reply >::what();
    }
}
