/*
 * amqp_config.cc
 *
 *  Created on: Jun 26, 2019
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "amqp_config.hh"

#include "application.hh"

#include "logger.hh"

using scarab::param_node;

LOGGER( dlog, "agent_config" );

namespace dripline
{

    amqp_config::amqp_config()
    {
        // default amqp configuration

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
        add( "max-payload-size", DL_MAX_PAYLOAD_SIZE );
    }

    amqp_config::~amqp_config()
    {
    }

    void add_amqp_options( scarab::main_app& an_app )
    {
        an_app.add_config_option< std::string >( "-b,--broker", "amqp.broker", "Set the dripline broker address" );
        an_app.add_config_option< unsigned >( "-p,--port", "amqp.broker-port", "Set the port for communication with the dripline broker" );
        an_app.add_config_option< std::string >( "-a,--auth-file", "amqp.auth-file", "Set the authentication file path" );
        an_app.add_config_option< unsigned >( "--max-payload", "amqp.max-payload-size", "Set the maximum payload size (in bytes)" );
        return;
    }

} /* namespace dripline */
