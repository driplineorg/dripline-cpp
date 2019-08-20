/*
 * labview_service.cc
 *
 *  Created on: Aug 20, 2019
 *      Author: N.S. Oblath
 */


#include "agent_config.hh"
#include "dripline_constants.hh"
#include "dripline_error.hh"
#include "dripline_version.hh"
#include "labview_service.hh"

#include "application.hh"
#include "logger.hh"
#include "signal_handler.hh"

using namespace dripline;

LOGGER( dlog, "simple_service" );

int main( int argc, char** argv )
{
    scarab::main_app the_main;

    the_main.set_version( new dripline::version() );

    the_main.default_config() = agent_config();

    int the_return = -1;

    auto t_service_callback = [&](){
        scarab::param_node& t_config = the_main.master_config()["amqp"].as_node();
        t_config.add( "queue", "labview" );
        labview_service the_service( t_config );

        scarab::signal_handler t_sig_hand;
        t_sig_hand.add_cancelable( &the_service );

        int t_return = RETURN_SUCCESS;
        try
        {
            if( ! the_service.start() ) throw dripline_error() << "Unable to start service";

            if( ! the_service.listen() ) throw dripline_error() << "Unable to start listening";

            if( ! the_service.stop() ) throw dripline_error() << "Unable to stop service";
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Exception caught: " << e.what() );
            t_return = RETURN_ERROR;
        }

        the_return = t_return;
    };

    the_main.callback( t_service_callback );

    CLI11_PARSE( the_main, argc, argv );

    return the_return;
}



