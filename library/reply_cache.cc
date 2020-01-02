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
    void set_reply_cache( const throw_reply& a_throw )
    {
        reply_cache::get_instance()->operator=( a_throw );
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
