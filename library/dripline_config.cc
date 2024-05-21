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

    dripline_config::dripline_config( const std::string& a_auth_file )
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

    dripline_config::~dripline_config()
    {
    }

    void add_dripline_options( scarab::main_app& an_app )
    {
        an_app.add_config_option< std::string >( "-b,--broker", "dripline.broker", "Set the dripline broker address" );
        an_app.add_config_option< unsigned >( "-p,--port", "dripline.broker-port", "Set the port for communication with the dripline broker" );
        an_app.add_config_option< std::string >( "--auth-file", "dripline.auth-file", "Set the authentication file path" );
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

    void add_dripline_auth_spec( scarab::main_app& an_app )
    {
        an_app.add_default_auth_spec_group( "dripline",
            scarab::param_node( 
                "user"_a=scarab::param_node(
                    "default"_a="guest",
                    "env"_a="DRIPLINE_USER"
                ),
                "password"_a=scarab::param_node(
                    "default"_a="guest",
                    "env"_a="DRIPLINE_PASSWORD"
                )
            )
        );
        return;
    }

} /* namespace dripline */
