/*
 * dripline_config.cc
 *
 *  Created on: Jun 26, 2019
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "dripline_config.hh"

#include "application.hh"

#include "logger.hh"

using scarab::param_node;

LOGGER( dlog, "agent_config" );

namespace dripline
{

    dripline_config::dripline_config()
    {
        // default dripline configuration

        add( "broker-port", 5672 );
        add( "broker", "localhost" );
#ifdef DRIPLINE_AUTH_FILE
        //add logic for default auth file if it exists
        scarab::path t_auth_default_path = scarab::expand_path( TOSTRING( DRIPLINE_AUTH_FILE ) );
        if ( boost::filesystem::exists( t_auth_default_path ) )
        {
            LDEBUG( dlog, "default auth file found, setting that as initial value: " << t_auth_default_path.string() );
            add( "auth-file", t_auth_default_path.string() );
        }
        else
        {
            LDEBUG( dlog, "default auth file <" << t_auth_default_path.string() << "> does not exist, not setting" );
        }
#endif
        add( "requests-exchange", "requests" );
        add( "alerts-exchange", "alerts" );
        add( "max-payload-size", DL_MAX_PAYLOAD_SIZE );
        add( "listen-timeout-ms", 1000 );
        add( "message-wait-ms", 1000 );
        add( "heartbeat-routing-key", "heartbeat" );
        add( "heartbeat-interval-s", 60 );
    }

    dripline_config::~dripline_config()
    {
    }

    void add_dripline_options( scarab::main_app& an_app )
    {
        an_app.add_config_option< std::string >( "-b,--broker", "dripline.broker", "Set the dripline broker address" );
        an_app.add_config_option< unsigned >( "-p,--port", "dripline.broker-port", "Set the port for communication with the dripline broker" );
        an_app.add_config_option< std::string >( "-a,--auth-file", "dripline.auth-file", "Set the authentication file path" );
        an_app.add_config_option< unsigned >( "--max-payload", "dripline.max-payload-size", "Set the maximum payload size (in bytes)" );
        return;
    }

} /* namespace dripline */
