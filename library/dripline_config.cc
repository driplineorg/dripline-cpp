/*
 * dripline_config.cc
 *
 *  Created on: Jun 26, 2019
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "dripline_config.hh"

#include "application.hh"
#include "param_helpers_impl.hh"

#include "logger.hh"

using scarab::param_node;
using_param_args_and_kwargs;

LOGGER( dlog, "agent_config" );

namespace dripline
{

    dripline_config::dripline_config()
    {
        // default dripline configuration
        add( "broker-port", 5672 );
        add( "broker", "localhost" );
        add( "requests-exchange", "requests" );
        add( "alerts-exchange", "alerts" );
        add( "max-payload-size", DL_MAX_PAYLOAD_SIZE );
        add( "loop-timeout-ms", 1000 );
        add( "message-wait-ms", 1000 );
        add( "heartbeat-routing-key", "heartbeat" );
        add( "heartbeat-interval-s", 60 );
        add( "max-connection-attempts", 10 );
    }

    void add_dripline_options( scarab::main_app& an_app )
    {
        // authentication for the broker
        an_app.add_config_option< std::string >( "-u,--username", "auth-groups.dripline.username.value", "Specify the username for the rabbitmq broker" );
        an_app.add_config_option< std::string >( "--password", "auth-groups.dripline.password.value", "Specify a password for the rabbitmq broker -- NOTE: this will be plain text on the command line and may end up in your command history!" );
        an_app.add_config_option< std::string >( "--password-file", "auth-groups.dripline.password.file", "Specify a file (e.g. a secrets file) to be read in as the rabbitmq broker password" );
        an_app.add_config_option< std::string >( "--auth-file", "auth-file", "Set the authentication file path" );

        // other dripline things
        an_app.add_config_option< std::string >( "-b,--broker", "dripline.broker", "Set the dripline broker address" );
        an_app.add_config_option< unsigned >( "-p,--port", "dripline.broker-port", "Set the port for communication with the dripline broker" );
        an_app.add_config_option< std::string >( "--requests-exchange", "dripline.requests-exchange", "Set the name of the requests exchange" );
        an_app.add_config_option< std::string >( "--alerts-exchange", "dripline.alerts-exchange", "Set the name of the alerts exchange" );
        an_app.add_config_option< unsigned >( "--max-payload", "dripline.max-payload-size", "Set the maximum payload size (in bytes)" );
        an_app.add_config_option< unsigned >( "--loop-timeout-ms" "dripline.loop-timeout-ms", "Set the timeout for thread loops in ms" );
        an_app.add_config_option< unsigned >( "--message-wait-ms" "dripline.message-wait-ms", "Set the time to wait for a full multi-part message in ms" );
        an_app.add_config_option< std::string >( "--heartbeat-routing-key", "dripline.heartbeat-routing-key", "Set the first token of heartbeat routing keys: [token].[origin]" );
        an_app.add_config_option< unsigned >( "--heartbeat-interval-s", "dripline.heartbeat-interval-s", "Set the interval between heartbeats in s" );
        an_app.add_config_option< unsigned >( "--max-connection-attempts", "dripline.max-connection-attempts", "Maximum number of times to attempt to connect to the broker" );

        return;
    }

    scarab::param_node create_dripline_auth_spec()
    {
        return scarab::param_node( 
            "username"_a=scarab::param_node(
                "default"_a="guest",
                "env"_a="DRIPLINE_USER"
            ),
            "password"_a=scarab::param_node(
                "default"_a="guest",
                "env"_a="DRIPLINE_PASSWORD"
            )
        );
    }

    void add_dripline_auth_spec( scarab::main_app& an_app, bool a_use_auth_file )
    {
        // This is setup as an either-or feature:
        //   You're defaults are either the auth-specification or an auth file
        // The use of an auth file is being maintained for backwards compatibility, 
        // but is not the preferred method of handling authentication
        if( a_use_auth_file )
        {
            an_app.set_default_auth_file( "authentications.json" );
        }
        else
        {
            an_app.add_default_auth_spec_group( "dripline", create_dripline_auth_spec() );
        }
        return;
    }

} /* namespace dripline */
