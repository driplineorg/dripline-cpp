/*
 * agent_config.cc
 *
 *  Created on: Jun 2, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "agent_config.hh"

#include "macros.hh"

using std::string;
using scarab::param_value;

namespace dripline
{

    agent_config::agent_config()
    {
        // default agent configuration

        param_node t_amqp_node;
        t_amqp_node.add( "broker-port", 5672 );
        t_amqp_node.add( "broker", "localhost" );
        t_amqp_node.add( "reply-timeout-ms", 10000 );
#ifdef DRIPLINE_AUTH_FILE
        t_amqp_node.add( "auth-file", TOSTRING( DRIPLINE_AUTH_FILE ) );
#endif
        add( "amqp", std::move(t_amqp_node) );
    }

    agent_config::~agent_config()
    {
    }

} /* namespace dripline */
