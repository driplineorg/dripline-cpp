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

    reply_cache::reply_cache() : 
        throw_reply()
    {}

    reply_cache::~reply_cache()
    {}

    reply_cache& reply_cache::operator=( const reply_cache& a_orig )
    {
        std::unique_lock< std::mutex >( f_mutex );
        throw_reply::operator=( a_orig );
        return *this;
    }

} /* namespace dripline */
