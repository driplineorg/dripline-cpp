/*
 * run_simple_service.cc
 *
 *  Created on: Aug 23, 2018
 *      Author: N.S. Oblath
 */

#define DRIPLINE_EXAMPLES_API_EXPORTS

#include "run_simple_service.hh"

#include "dripline_error.hh"

#include "logger.hh"
#include "signal_handler.hh"

#include <chrono>
#include <thread>

LOGGER( dlog, "run_simple_service" )

namespace dripline
{

    run_simple_service::run_simple_service( const scarab::param_node& a_config ) :
            service( a_config, "simple" ),
            f_return( RETURN_SUCCESS )
    {
    }

    run_simple_service::~run_simple_service()
    {
    }

    void run_simple_service::execute()
    {
        scarab::signal_handler t_sig_hand;
        t_sig_hand.add_cancelable( this );

        try
        {
            if( ! start() ) throw dripline_error() << "Unable to start service";

            if( ! listen() ) throw dripline_error() << "Unable to start listening";

            if( ! stop() ) throw dripline_error() << "Unable to stop service";
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Exception caught: " << e.what() );
            f_return = RETURN_ERROR;
        }

        return;
    }

} /* namespace dripline */
