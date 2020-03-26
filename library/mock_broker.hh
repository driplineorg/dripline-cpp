/*
 * mock_broker.hh
 *
 *  Created on: Mar 24, 2020
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_MOCK_BROKER_HH_
#define DRIPLINE_MOCK_BROKER_HH_

#include "endpoint.hh"

#include "concurrent_queue.hh"
#include "singleton.hh"

#include <map>
#include <set>

namespace dripline
{

    /*!
     @class mock_broker
     @author N.S. Oblath

     @brief 

     @details
    */
    class DRIPLINE_API mock_broker : public scarab::singleton< mock_broker >
    {
        public:
            void send( message_ptr_t a_message, const std::string& an_exchange );

            // consumer tag should be the queue name
            message_ptr_t consume( const std::string& a_consumer_tag );

            void declare_exchange( const std::string& an_exchange );

            void declare_queue( const std::string& a_queue );

            // throws dripline_error(?) if exchange or queue do not exist
            void bind( const std::string& an_exchange, const std::string& a_queue, const std::string& a_key );

            void unbind( const std::string& an_exchange, const std::string& a_queue, const std::string& a_key );

            void remove_queue( const std::string& an_exchange, const std::string& a_queue );

        protected:
            allow_singleton_access( mock_broker );

            mock_broker();
            mock_broker( const mock_broker& a_orig ) = delete;
            mock_broker( mock_broker&& a_orig ) = delete;
            virtual ~mock_broker();

            mock_broker& operator=( const mock_broker& a_orig ) = delete;
            mock_broker& operator=( mock_broker&& a_orig ) = delete;

            typedef scarab::concurrent_queue< message_ptr_t > queue;
            typedef std::shared_ptr< queue > queue_ptr_t;

            mv_referrable( std::map< std::string, queue_ptr_t >, queues );
            mv_referrable( std::set< std::string >, exchanges );
            mv_referrable( std::map< std::string, queue_ptr_t >, routing_keys );

    }

} /* namespace dripline */

#endif /*  DRIPLINE_MOCK_BROKER_HH_ */