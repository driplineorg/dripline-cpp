/*
 *  reply_cache.hh
 *
 *  Created on: Dec 31, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_REPLY_CACHE_HH_
#define DRIPLINE_REPLY_CACHE_HH_

#include "throw_reply.hh"

#include "thread_singleton.hh"

namespace dripline
{

    /// Set the contents of the reply cache (functional access to the singleton reply_cache)
    void DRIPLINE_API set_reply_cache( const return_code& a_code, const std::string& a_message, scarab::param_ptr_t a_payload_ptr );

    /*!
     @class reply_cache
     @author N.S. Oblath

     @brief A singleton throw_reply object used to transfer throw_reply information to C++ from other implementations (e.g. Python)

     @details
     Example usage:
     In Python, when a ThrowReply is raised, that's eventually converted automatically to an thrown exceptoin of type reply_already_set.  
     Since we have no control over that conversion, we use the reply_cache to first store all of the information 
     in the ThrowReply, before the C++ exception is raised.  Once the exception is raised and caught, we can 
     recognize that the origin was a Python ThrowReply, and then grab the information out of the reply_cache to send the reply.

     For more details, see endpoint::on_request_message()'s technique for recognizing the type of the Python exception, 
     and dripline-python's use of the cache in ThrowReply.py.
    */
    class DRIPLINE_API reply_cache : public throw_reply, public scarab::thread_singleton< reply_cache >
    {
        public:
            reply_cache& operator=( const throw_reply& a_orig );

            /// Set the contents of the reply cache (thread-safe)
            void set_cache( const return_code& a_code, const std::string& a_message, scarab::param_ptr_t a_payload_ptr );

        protected:
            allow_thread_singleton_access( reply_cache );
            reply_cache();
            reply_cache( const reply_cache& ) = delete;
            virtual ~reply_cache() noexcept;
    };

} /* namespace dripline */

#endif /* DRIPLINE_REPLY_CACHE_HH_ */
