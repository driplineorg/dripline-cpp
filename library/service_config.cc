/*
 * service_config.cc
 *
 *  Created on: Aug 24, 2024
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "service_config.hh"

#include "application.hh"
#include "param_helpers_impl.hh"

//#include "logger.hh"

//LOGGER( dlog, "service_config" );

namespace dripline
{

    service_config::service_config( const std::string& a_name )
    {
        // default service configuration

        add( "dripline_mesh", dripline_config() );

        add( "name", a_name );
        add( "loop_timeout_ms", 1000 );
        add( "message_wait_ms", 1000 );
        add( "heartbeat_interval_s", 60 );
    }

    void add_service_options( scarab::main_app& an_app )
    {
        an_app.add_config_option< unsigned >( "--loop-timeout-ms" "loop_timeout_ms", "Set the timeout for thread loops in ms" );
        an_app.add_config_option< unsigned >( "--message-wait-ms" "message_wait_ms", "Set the time to wait for a full multi-part message in ms" );
        an_app.add_config_option< unsigned >( "--heartbeat-interval-s", "heartbeat_interval_s", "Set the interval between heartbeats in s" );
        return;
    }

} /* namespace dripline */
