/*
 * run_oscillator_service_endpoints.cc
 *
 *  Created on: May 21, 2019
 *      Author: N.S. Oblath
 */

#include "agent_config.hh"
#include "dripline_constants.hh"
#include "dripline_version.hh"
#include "oscillator_service_endpoints.hh"

#include "application.hh"
#include "logger.hh"

using namespace dripline;

LOGGER( dlog, "run_oscillation_service_endpoints" );

int main( int argc, char** argv )
{
    scarab::main_app the_main;

    the_main.set_version( new dripline::version() );

    the_main.default_config() = agent_config();

    oscillator_service_endpoints the_service;

    auto t_service_callback = [&](){
        the_service.execute();
    };

    the_main.callback( t_service_callback );

    CLI11_PARSE( the_main, argc, argv );

    return the_service.get_return();
}



