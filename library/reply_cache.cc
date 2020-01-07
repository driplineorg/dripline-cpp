/*
 *  reply_cache.cc
 *
 *  Created on: Dec 31, 2019
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "reply_cache.hh"

namespace dripline
{

    void DRIPLINE_API set_reply_cache( const return_code& a_code, const std::string& a_message, scarab::param_ptr_t a_payload_ptr )
    {
        reply_cache::get_instance()->set_return_code( a_code );
        reply_cache::get_instance()->return_message() = a_message;
        reply_cache::get_instance()->set_payload( std::move(a_payload_ptr) );
        return;
    }

    reply_cache::reply_cache() : 
        throw_reply()
    {}

    reply_cache::~reply_cache() noexcept
    {}

    reply_cache& reply_cache::operator=( const throw_reply& a_orig )
    {
        std::unique_lock< std::mutex > t_lock( f_mutex );
        throw_reply::operator=( a_orig );
        return *this;
    }

} /* namespace dripline */
