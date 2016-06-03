/*
 * dl_agent.cc
 *
 *      Author: Noah Oblath
 *
 *  Dripline agent
 *
 *  Usage:
 *  $> dl_agent [operation] [options]
 *
 */

#define DRIPLINE_API_EXPORTS
#define SCARAB_API_EXPORTS

#include "agent.hh"
#include "agent_config.hh"
#include "dripline_constants.hh"
#include "dripline_version.hh"

#include "configurator.hh"
#include "logger.hh"

using namespace dripline;


LOGGER( dlog, "mantis_client" );

set_version( dripline, version );

int main( int argc, char** argv )
{
    dripline::version_setter s_vsetter_mantis_version( new dripline::version() );
    try
    {
        agent_config t_cc;
        scarab::configurator t_configurator( argc, argv, &t_cc );

        // Run the client

        agent the_agent( t_configurator.config() );

        the_agent.execute();

        return the_agent.get_return();
    }
    catch( scarab::error& e )
    {
        LERROR( dlog, "configuration error: " << e.what() );
        return RETURN_ERROR;
    }
    catch( std::exception& e )
    {
        LERROR( dlog, "std::exception caught: " << e.what() );
        return RETURN_ERROR;
    }

    return RETURN_ERROR;
}
