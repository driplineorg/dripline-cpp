/*
 * mock_broker.cc
 *
 *  Created on: Mar 25, 2020
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS


#include "mock_broker.hh"

#include "dripline_exceptions.hh"

#include "logger.hh"


LOGGER( dlog, "mock_broker" );

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
        if( f_exchanges.count( an_exchange ) == 0 )
        {
            LWARN( dlog, "Exchange <" << an_exchange << "> does not exist" );
            throw dripline_error() << "Cannot send message:\n"
                << "\tExchange <" << an_exchange << "> does not exist";
        }

        std::string& t_key( a_message->routing_key() );
        auto t_bindings_it = f_routing_keys[ an_exchange ].find( t_key );
        if( t_bindings_it == f_routing_keys[ an_exchange ].end() )
        {
            LWARN( dlog, "No binding for key <" << t_key << ">" );
            throw dripline_error() << "Cannot send message:\n"
                << "\tNo binding for key <" << t_key << ">";
        }

        t_bindings_it->second->push( a_message );

        return;
    }

    message_ptr_t mock_broker::consume( const std::string& a_consumer_tag, unsigned a_timeout_ms )
    {
        // find queue
        auto t_queue_it = f_queues.find( a_consumer_tag );
        if( t_queue_it == f_queues.end() )
        {
            LWARN( dlog, "Queue <" << a_consumer_tag << "> does not exist" );
            throw dripline_error() << "Cannot consume message with consumer tag <" << a_consumer_tag << ">:\n"
                << "\tQueue does not exist";
        }

        if( a_timeout_ms != 0 )
        {
            t_queue_it->second->set_timeout( a_timeout_ms );
        }

        message_ptr_t t_retrieved_message;
        bool t_success = t_queue_it->second->timed_wait_and_pop( t_retrieved_message );

        if( ! t_success )
        {
            LWARN( dlog, "Did not retrieve message from queue <" << t_queue_it->first << ">");
            throw dripline_error() << "Did not retrieve message from queue <" << t_queue_it->first << ">:\n"
                << "\tEither wait timed out or queue was interrupted";
        }

        return t_retrieved_message;
    }

    void mock_broker::declare_exchange( const std::string& an_exchange )
    {
        f_exchanges.insert( an_exchange );
        return;
    }

    void mock_broker::declare_queue( const std::string& a_queue )
    {
        f_queues.insert( map_to_queues_t::value_type( std::make_pair(a_queue, queue_ptr_t(new queue_t)) ) );
        return;
    }

    void mock_broker::bind( const std::string& an_exchange, const std::string& a_queue, const std::string& a_key )
    {
        LDEBUG( dlog, "Binding: <" << an_exchange << "--(" << a_key << ")-->" << a_queue << ">")

        // find exchange
        if( f_exchanges.count( an_exchange ) == 0 )
        {
            LWARN( dlog, "Exchange <" << an_exchange << "> does not exist" );
            throw dripline_error() << "Cannot create binding <" << an_exchange << "--(" << a_key << ")-->" << a_queue << ">:\n"
                << "\tExchange does not exist";
        }

        // find queue
        if( f_queues.count( a_queue) == 0 )
        {
            LWARN( dlog, "Queue <" << a_queue << "> does not exist" );
            throw dripline_error() << "Cannot create binding <" << an_exchange << "--(" << a_key << ")-->" << a_queue << ">:\n"
                << "\tQueue does not exist";
        }
        queue_ptr_t t_queue_to_bind = f_queues.at( a_queue );

        // check for existing binding
        if( f_routing_keys[ an_exchange ].count( a_key ) != 0 )
        {
            LWARN( dlog, "Unable to create binding; routing key <" << a_key << "> is already in use" );
            throw dripline_error() << "Cannot create binding <" << an_exchange << "--(" << a_key << ")-->" << a_queue << ">:\n"
                << "\tRouting key is already in use ";
        }

        f_routing_keys[ an_exchange ][ a_key ] = t_queue_to_bind;
        LINFO( dlog, "Bound: <" << an_exchange << "--(" << a_key << ")-->" << a_queue << ">")

        return;
    }

    void mock_broker::unbind( const std::string& an_exchange, const std::string& a_queue, const std::string& a_key )
    {
        LDEBUG( dlog, "Unbinding: <" << an_exchange << "--(" << a_key << ")-->" << a_queue << ">")

        // find exchange
        if( f_exchanges.count( an_exchange ) == 0 || f_routing_keys.count( an_exchange ) == 0 )
        {
            LWARN( dlog, "Exchange <" << an_exchange << "> does not exist" );
            throw dripline_error() << "Cannot unbind <" << an_exchange << "--(" << a_key << ")-->" << a_queue << ">:\n"
                << "\tExchange does not exist or has no bindings";
        }

        // find queue
        if( f_queues.count( a_queue) == 0 )
        {
            LWARN( dlog, "Queue <" << a_queue << "> does not exist" );
            throw dripline_error() << "Cannot unbind <" << an_exchange << "--(" << a_key << ")-->" << a_queue << ">:\n"
                << "\tQueue does not exist";
        }

        // check for existing binding
        auto t_exchange_bindings_it = f_routing_keys.find( an_exchange );
        // we already checked that f_routing_keys has an_exchange, so we don't need to check whether it was found
        if( t_exchange_bindings_it->second.count( a_key ) == 0 )
        {
            LWARN( dlog, "Routing key <" << a_key << "> is not in use for this binding" );
            throw dripline_error() << "Cannot unbind <" << an_exchange << "--(" << a_key << ")-->" << a_queue << ">:\n"
                << "\tRouting key is not in use ";
        }
        t_exchange_bindings_it->second.erase( a_key );

        // check whether that was the last binding on the exchange
        if( t_exchange_bindings_it->second.empty() )
        {
            LDEBUG( dlog, "Last routing key for exchange <" << an_exchange << "> has been unbound" );
            f_routing_keys.erase( an_exchange );
        }

        LINFO( dlog, "Unbound: <" << an_exchange << "--(" << a_key << ")-->" << a_queue << ">" )

        return;
    }

    void mock_broker::delete_queue( const std::string& a_queue )
    {
        LINFO( dlog, "Deleting queue <" << a_queue << ">" );

        auto t_this_queue_it = f_queues.find( a_queue );
        // if the queue doesn't exist, we can just quit
        if( t_this_queue_it == f_queues.end() ) return;

        // unbind queue if bound to any exchanges
        for( auto t_exchange_it = f_routing_keys.begin(); t_exchange_it != f_routing_keys.end(); )
        {
            LWARN( dlog, "exchange <" << t_exchange_it->first << "> has <" << t_exchange_it->second.size() << "> entries" );
            for( auto t_binding_it = t_exchange_it->second.begin(); t_binding_it != t_exchange_it->second.end(); )
            {
                if( t_this_queue_it->second == t_binding_it->second )
                {
                    LDEBUG( dlog, "Unbinding <" << t_exchange_it->first << "--(" << t_binding_it->first << ")-->" << a_queue << ">" );
                    t_binding_it = t_exchange_it->second.erase( t_binding_it );
                }
                else
                {
                    ++t_binding_it;
                }
                
            }
            // remove exchange from the bindings map 
            if( t_exchange_it->second.empty() )
            {
                LDEBUG( dlog, "Last routing key for exchange <" << t_exchange_it->first << "> has been unbound" );
                t_exchange_it = f_routing_keys.erase( t_exchange_it );
            }
            else
            {
                ++t_exchange_it;
            }
        }

        // finally, remove the queue
        f_queues.erase( t_this_queue_it );

        return;
    }

    void mock_broker::delete_exchange( const std::string& an_exchange )
    {
        LINFO( dlog, "Deleting exchange <" << an_exchange << ">" );

        if( f_exchanges.count( an_exchange ) == 0 ) return;

        auto t_exchange_it = f_routing_keys.find( an_exchange );
        if( t_exchange_it != f_routing_keys.end() )
        {
#ifndef NDEBUG
            for( auto t_binding_it = t_exchange_it->second.begin(); t_binding_it != t_exchange_it->second.end(); )
            {
                    LDEBUG( dlog, "Unbinding <" << t_exchange_it->first << "--(" << t_binding_it->first << ")-->[queue]>" );
                    t_binding_it = t_exchange_it->second.erase( t_binding_it );
            }
#endif
            // erase bindings map for this exchange
            f_routing_keys.erase( t_exchange_it );
        }

        // remove the exchange
        f_exchanges.erase( an_exchange );

        return;
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
