/*
 * agent_config.cc
 *
 *  Created on: Jun 2, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "agent_config.hh"

//#include "logger.hh"

//LOGGER( dlog, "agent_config" );

namespace dripline
{

    agent_config::agent_config()
    {
        // default agent configuration

        add( "dripline", dripline_config() );

        add( "timeout", 10 ); // seconds
    }

} /* namespace dripline */
