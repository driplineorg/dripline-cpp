/*
 * mock_broker.cc
 *
 *  Created on: Mar 25, 2020
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS


#include "mock_broker.hh"



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

} /* namespace dripline */
