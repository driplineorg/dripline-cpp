/*
 * simple_service.cc
 *
 *  Created on: Aug 23, 2018
 *      Author: N.S. Oblath
 */

#include "agent_config.hh"
#include "dripline_constants.hh"
#include "dripline_version.hh"
#include "run_simple_service.hh"

#include "application.hh"
#include "logger.hh"

using namespace dripline;

LOGGER( dlog, "simple_service" );

int main( int argc, char** argv )
{
    scarab::main_app the_main;

    the_main.set_version( new dripline::version() );

    the_main.default_config() = agent_config();

    // options
    //std::string t_broker;
    //the_main.add_option( "-b,--broker", , "RabbitMQ broker" );

    int the_return = -1;

    auto t_service_callback = [&](){
        run_simple_service the_service( the_main.master_config()["amqp"].as_node() );

        the_service.execute();

        the_return = the_service.get_return();
    };

    the_main.callback( t_service_callback );

    CLI11_PARSE( the_main, argc, argv );

    return the_return;
}
