/*
 * simple_service.cc
 *
 *  Created on: Aug 23, 2018
 *      Author: N.S. Oblath
 */

#define DRIPLINE_EXAMPLES_API_EXPORTS

#include "simple_service.hh"

#include "dripline_exceptions.hh"

#include "logger.hh"
#include "macros.hh"
#include "param.hh"
#include "signal_handler.hh"

#include <chrono>
#include <thread>

LOGGER( dlog, "simple_service" )

namespace dripline
{

    simple_service::simple_service( const scarab::param_node& a_config ) :
            scarab::cancelable(),
            service( a_config, "simple" ),
            f_return( dl_success().rc_value() )
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
            f_return = dl_service_error().rc_value() / 100;
        }

        if( scarab::signal_handler::get_exited() )
        {
            f_return = scarab::signal_handler::get_return_code();
        }

        return;
    }

    reply_ptr_t simple_service::do_cmd_request( const request_ptr_t a_request )
    {
        if( a_request->parsed_specifier().empty() )
        {
            return a_request->reply( dl_service_error_invalid_specifier(), "No specifier provided" );
        }

        std::string t_specifier = a_request->parsed_specifier().front();
        a_request->parsed_specifier().pop_front();

        if( t_specifier == "echo" )
        {
            reply_ptr_t t_reply = a_request->reply( dl_success(), "Echoed payload" );
            LDEBUG( dlog, "Echoing payload: \n" << a_request->payload() );
            t_reply->set_payload( a_request->payload().clone() );
            return t_reply;
        }
        else if( t_specifier == "error" )
        {
            throw std::runtime_error( "An error occurred in the endpoint!  (Note: this is a test, this is only a test)" );
        }
        else
        {
            return a_request->reply( dl_service_error_invalid_specifier(), "Unknown specifier: " + t_specifier );
        }
    }



} /* namespace dripline */
