/*
 * agent_config.cc
 *
 *  Created on: Jun 2, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "agent_config.hh"

#include "logger.hh"

using std::string;

LOGGER( dlog, "agent_config" );

namespace dripline
{

    agent_config::agent_config()
    {
        // default agent configuration

        add( "amqp", amqp_config() );

        (*this)["amqp"].as_node().add( "timeout", 10 );
        (*this)["amqp"].as_node().add( "exchange", "requests" );
    }

    agent_config::~agent_config()
    {
    }

} /* namespace dripline */
