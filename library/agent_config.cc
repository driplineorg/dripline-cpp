/*
 * agent_config.cc
 *
 *  Created on: Jun 2, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "agent_config.hh"

#include "logger.hh"
#include "macros.hh"

using std::string;
using scarab::param_value;

LOGGER( dlog, "agent_config" );

namespace dripline
{

    agent_config::agent_config()
    {
        // default agent configuration

        param_node t_amqp_node;
        t_amqp_node.add( "broker-port", 5672 );
        t_amqp_node.add( "broker", "localhost" );
        t_amqp_node.add( "timeout", 10 );
#ifdef DRIPLINE_AUTH_FILE
        //add logic for default auth file if it exists
        scarab::path t_auth_default_path = scarab::expand_path( TOSTRING( DRIPLINE_AUTH_FILE ) );
        if ( boost::filesystem::exists( t_auth_default_path ) )
        {
            LDEBUG( dlog, "default auth file found, setting that as initial value: " << t_auth_default_path.string() );
            t_amqp_node.add( "auth-file", t_auth_default_path.string() );
        }
        else
        {
            LDEBUG( dlog, "default auth file <" << t_auth_default_path.string() << "> does not exist, not setting" );
        }
#endif
        add( "amqp", t_amqp_node );
    }

    agent_config::~agent_config()
    {
    }

} /* namespace dripline */
