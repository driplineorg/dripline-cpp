/*
 * throw_reply.cc
 *
 *  Created on: Jan 2, 2020
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "throw_reply.hh"

namespace dripline
{
#ifdef DL_PYTHON
    std::string throw_reply::s_py_throw_reply_keyword( TOSTRING(PYTHON_THROW_REPLY_KEYWORD) );
#endif

    throw_reply::throw_reply() :
            f_return_message(),
            f_return_code( new dl_unhandled_exception() ),
            f_payload( new scarab::param() )
    {}

    throw_reply::throw_reply( const return_code& a_code, scarab::param_ptr_t a_payload_ptr ) :
            f_return_message(),
            f_return_code( new copy_code( a_code ) ),
            f_payload( std::move(a_payload_ptr) )
    {}

    throw_reply::throw_reply( const throw_reply& a_orig ) :
            f_return_message( a_orig.f_return_message ),
            f_return_code( a_orig.f_return_code ),
            f_payload( a_orig.f_payload->clone() )
    {}

    throw_reply::~throw_reply() noexcept
    {}

    throw_reply& throw_reply::operator=( const throw_reply& a_orig )
    {
        f_return_message = a_orig.f_return_message;
        f_return_code = a_orig.f_return_code;
        f_payload = a_orig.f_payload->clone();
        return *this;
    }

}
