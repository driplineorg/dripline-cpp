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
            typedef scarab::concurrent_queue< message_ptr_t > queue_t;
            typedef std::shared_ptr< queue_t > queue_ptr_t;

            void send( message_ptr_t a_message, const std::string& an_exchange );

            // consumer tag should be the queue name
            message_ptr_t consume( const std::string& a_consumer_tag );

            void declare_exchange( const std::string& an_exchange );

            void declare_queue( const std::string& a_queue );

            // throws dripline_error(?) if exchange or queue do not exist
            void bind( const std::string& an_exchange, const std::string& a_queue, const std::string& a_key );

            void unbind( const std::string& an_exchange, const std::string& a_queue, const std::string& a_key );

            void remove_queue( const std::string& an_exchange, const std::string& a_queue );

            bool is_bound( const std::string& an_exchange, const std::string& a_queue, const std::string& a_key ) const;

        protected:
            allow_singleton_access( mock_broker );

            mock_broker();
            mock_broker( const mock_broker& a_orig ) = delete;
            mock_broker( mock_broker&& a_orig ) = delete;
            virtual ~mock_broker();

            mock_broker& operator=( const mock_broker& a_orig ) = delete;
            mock_broker& operator=( mock_broker&& a_orig ) = delete;

            typedef std::map< std::string, queue_ptr_t > map_to_queues_t;
            typedef std::set< std::string > string_set_t;

            mv_referrable( map_to_queues_t, queues );  // queue name --> queue  (to return or remove a queue)
            mv_referrable( string_set_t, exchanges );  // exchange name (to return or remove an exchange)

            typedef std::map< std::string, map_to_queues_t > bindings_map_t;
            mv_referrable( bindings_map_t, routing_keys );  // exchange --> routing key --> queue  (to route a message or unbind a key)

    };

} /* namespace dripline */

#endif /*  DRIPLINE_MOCK_BROKER_HH_ */