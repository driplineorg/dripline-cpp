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

    dripline_config::dripline_config( const std::string& a_auth_file )
    {
        // default dripline configuration

        scarab::path t_auth_default_path;
#ifdef DRIPLINE_AUTH_FILE
        //add logic for default auth file if it exists
        LTRACE( dlog, "Have a default auth path provided at build time: <" << TOSTRING( DRIPLINE_AUTH_FILE ) << ">" );
        t_auth_default_path = scarab::expand_path( TOSTRING( DRIPLINE_AUTH_FILE ) );
#endif
        if( ! a_auth_file.empty() )
        {
            LTRACE( dlog, "Have a default auth path provided at runtime: <" << a_auth_file << ">" );
            t_auth_default_path = scarab::expand_path( a_auth_file );
        }

        if ( ! t_auth_default_path.empty() && boost::filesystem::exists( t_auth_default_path ) )
        {
            LDEBUG( dlog, "default auth file found, setting that as initial value: " << t_auth_default_path.string() );
            add( "auth-file", t_auth_default_path.string() );
        }
        else
        {
            if( t_auth_default_path.empty() )
            {
                LDEBUG( dlog, "No default auth file present; will not be set in dripline_config" );
            }
            else
            {
                LDEBUG( dlog, "Default auth path <" << t_auth_default_path.string() << "> does not exist; will not be set in dripline_config" );
            }
        }
        add( "requests-exchange", "requests" );
        add( "alerts-exchange", "alerts" );
        add( "max-payload-size", DL_MAX_PAYLOAD_SIZE );
        add( "loop-timeout-ms", 1000 );
        add( "message-wait-ms", 1000 );
        add( "heartbeat-routing-key", "heartbeat" );
        add( "heartbeat-interval-s", 60 );

        // broker and broker-port can be specified in the config.
        // however, we don't give default values so that they can be specified in the auth file.
        // the dripline config will override the auth file if it's given there.
        //add( "broker-port", 5672 );
        //add( "broker", "localhost" );

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

        return;
    }

} /* namespace dripline */
