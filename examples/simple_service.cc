/*
 * simple_service.cc
 *
 *  Created on: Aug 23, 2018
 *      Author: N.S. Oblath
 */

#include "simple_service.hh"

#include "logger.hh"
#include "signal_handler.hh"

#include <chrono>
#include <thread>

LOGGER( dlog, "simple_service" )

namespace dripline
{

    simple_service::simple_service( const scarab::param_node& a_config ) :
            service( a_config, "simple" ),
            scarab::cancelable(),
            f_return( RETURN_SUCCESS )
    {
    }

    simple_service::~simple_service()
    {
    }

    void simple_service::execute()
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
