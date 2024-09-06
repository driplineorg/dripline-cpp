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
        reply_cache::get_instance()->set_cache( a_code, a_message, std::move(a_payload_ptr) );
        return;
    }

    reply_cache::reply_cache() : 
        throw_reply()
    {}

    reply_cache::reply_cache( reply_cache&& a_orig) :
        throw_reply()
    {
        std::unique_lock< std::mutex > t_lock( f_mutex );
        throw_reply::operator=( std::move(a_orig) );
    }

    reply_cache& reply_cache::operator=( reply_cache&& a_orig )
    {
        std::unique_lock< std::mutex > t_lock( f_mutex );
        throw_reply::operator=( std::move(a_orig) );
        return *this;
    }

    reply_cache& reply_cache::operator=( const throw_reply& a_orig )
    {
        std::unique_lock< std::mutex > t_lock( f_mutex );
        throw_reply::operator=( a_orig );
        return *this;
    }

    void reply_cache::set_cache( const return_code& a_code, const std::string& a_message, scarab::param_ptr_t a_payload_ptr )
    {
        std::unique_lock< std::mutex > t_lock( f_mutex );
        reply_cache::get_instance()->set_return_code( a_code );
        reply_cache::get_instance()->return_message() = a_message;
        reply_cache::get_instance()->set_payload( std::move(a_payload_ptr) );
        return;
    }


} /* namespace dripline */
