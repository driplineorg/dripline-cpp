/*
 * agent_config.cc
 *
 *  Created on: Jun 2, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "agent_config.hh"

#include "logger.hh"

#include<string>
using std::string;

using scarab::param_value;

namespace dripline
{

    agent_config::agent_config()
    {
        // default agent configuration

        param_node* t_amqp_node = new param_node();
        t_amqp_node->add( "broker-port", new param_value( 5672 ) );
        t_amqp_node->add( "broker", new param_value( "localhost" ) );
        t_amqp_node->add( "exchange", new param_value( "requests" ) );
        t_amqp_node->add( "reply-timeout-ms", new param_value( 10000 ) );
#ifdef DRIPLINE_AUTH_FILE
        t_amqp_node->add( "auth-file", new param_value( TOSTRING( DRIPLINE_AUTH_FILE ) ) );
#endif
        add( "amqp", t_amqp_node );
    }

    agent_config::~agent_config()
    {
    }

} /* namespace dripline */
