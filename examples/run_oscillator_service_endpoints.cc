/*
 * run_oscillator_service_endpoints.cc
 *
 *  Created on: May 21, 2019
 *      Author: N.S. Oblath
 */

#include "agent_config.hh"
#include "dripline_constants.hh"
#include "service_config.hh"
#include "version_store.hh"
#include "oscillator_service_endpoints.hh"

#include "application.hh"
#include "logger.hh"
#include "signal_handler.hh"

using namespace dripline;

LOGGER( dlog, "run_oscillation_service_endpoints" );

int main( int argc, char** argv )
{
    // Start handling signals
    scarab::signal_handler t_sig_hand;

    scarab::main_app the_main;

    the_main.set_version( version_store::get_instance()->versions().at("dripline-cpp") );

    the_main.default_config() = service_config( "osc_svc_ep" );

    add_dripline_options( the_main );

    add_dripline_auth_spec( the_main );

    int the_return = -1;

    auto t_service_callback = [&](){
        auto the_service = std::make_shared< oscillator_service_endpoints >( the_main.primary_config()["dripline"].as_node(), the_main.auth() );

        the_service->set_pointers();
        the_service->execute();

        the_return = the_service->get_return();
    };

    the_main.callback( t_service_callback );

    CLI11_PARSE( the_main, argc, argv );

    return the_return;
}



