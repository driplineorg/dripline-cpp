/*
 *  reply_cache.hh
 *
 *  Created on: Dec 31, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_REPLY_CACHE_HH_
#define DRIPLINE_REPLY_CACHE_HH_

#include "dripline_exceptions.hh"

#include "thread_local_singleton.hh"

namespace dripline
{

    void DRIPLINE_API set_reply_cache( const throw_reply& a_orig );

    class DRIPLINE_API reply_cache : public throw_reply, public scarab::thread_singleton< reply_cache >
    {
        public:
            reply_cache& operator=( const throw_reply& a_orig );

        protected:
            allow_thread_singleton_access( reply_cache );
            reply_cache();
            reply_cache( const reply_cache& ) = delete;
            virtual ~reply_cache() noexcept;
    };

} /* namespace dripline */

#endif /* DRIPLINE_REPLY_CACHE_HH_ */
