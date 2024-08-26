/*
 * monitor_config.cc
 *
 *  Created on: Jul 3, 2019
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "monitor_config.hh"

#include "logger.hh"

LOGGER( dlog, "agent_config" );

namespace dripline
{

    monitor_config::monitor_config()
    {
        // default agent configuration

        add( "dripline", dripline_config() );

    }

} /* namespace dripline */
