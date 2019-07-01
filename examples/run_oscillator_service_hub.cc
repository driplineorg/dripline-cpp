/*
 * run_oscillator_service_hub.cc
 *
 *  Created on: May 16, 2019
 *      Author: N.S. Oblath
 */

#include "agent_config.hh"
#include "dripline_constants.hh"
#include "dripline_version.hh"
#include "oscillator_service_hub.hh"

#include "application.hh"
#include "logger.hh"

using namespace dripline;

LOGGER( dlog, "run_oscillation_service_hub" );

int main( int argc, char** argv )
{
    scarab::main_app the_main;

    the_main.set_version( new dripline::version() );

    the_main.default_config().add( "dripline", dripline_config() );

    add_dripline_options( the_main );

    int the_return = -1;

    auto t_service_callback = [&](){
        auto the_service = std::make_shared< oscillator_service_hub >( the_main.master_config()["dripline"].as_node() );

        the_service->execute();

        the_return = the_service->get_return();
    };

    the_main.callback( t_service_callback );

    CLI11_PARSE( the_main, argc, argv );

    return the_return;
}



