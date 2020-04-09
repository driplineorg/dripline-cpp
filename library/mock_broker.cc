/*
 * mock_broker.cc
 *
 *  Created on: Mar 25, 2020
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS


#include "mock_broker.hh"

#include "dripline_exceptions.hh"

namespace dripline
{

    mock_broker::mock_broker() :
            f_queues(),
            f_exchanges(),
            f_routing_keys()
    {}

    mock_broker::~mock_broker()
    {}

    void mock_broker::send( message_ptr_t a_message, const std::string& an_exchange )
    {

    }

    void mock_broker::bind( const std::string& an_exchange, const std::string& a_queue, const std::string& a_key )
    {

    }

    void mock_broker::unbind( const std::string& an_exchange, const std::string& a_queue, const std::string& a_key )
    {

    }

    void mock_broker::remove_queue( const std::string& an_exchange, const std::string& a_queue )
    {

    }

    bool mock_broker::is_bound( const std::string& an_exchange, const std::string& a_queue, const std::string& a_key ) const
    {
        if( f_exchanges.count( an_exchange ) == 0 || f_queues.count( a_queue ) == 0 ) return false;

        auto t_exchange_it = f_routing_keys.find( an_exchange );
        if( t_exchange_it == f_routing_keys.end() ) return false;

        auto t_queue_it = t_exchange_it->second.find( a_key );
        if( t_queue_it == t_exchange_it->second.end() ) return false;

        // if the pointers in the bound map match hose in the queue map, then we're good to go.
        if( t_queue_it->second == f_queues.at(a_queue)) return true;
        return false;
    }

/*
    mock_broker::queue_ptr_t mock_broker::queue( const std::string& an_exchange, const std::string& a_queue )
    {
        auto t_exchange_it = f_exchanges.find( an_exchange );
        if( t_exchange_it == f_exchanges.end() )
        {
            throw dripline_error() << "Could not return queue <" << a_queue << "> because exchange <" << an_exchange << "> is not present";
        }

        auto t_queue_it = t_exchange_it->second.f_queues.find( a_queue );
        if( t_queue_it == t_exchange_it->second.f_queues.end() )
        {
            throw dripline_error() << "Did not find queue <" << a_queue << "> bound to exchange <" << an_exchange << ">";
        }
    }

    const mock_broker::queue_ptr_t mock_broker::queue( const std::string& an_exchange, const std::string& a_queue )
    {

    }
*/

} /* namespace dripline */
