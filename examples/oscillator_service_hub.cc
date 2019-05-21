/*
 * oscillator_service_hub.cc
 *
 *  Created on: May 16, 2019
 *      Author: N.S. Oblath
 */

#define DRIPLINE_EXAMPLES_API_EXPORTS

#include "oscillator_service_hub.hh"

#include "logger.hh"
#include "signal_handler.hh"

//#include <ctime>
#include <functional>

using scarab::param_array;
using scarab::param_node;
using scarab::param_ptr_t;

using std::placeholders::_1;

LOGGER( dlog, "oscillator_service_hub" );

namespace dripline
{

    oscillator_service_hub::oscillator_service_hub( const scarab::param_node& a_config ) :
            hub( a_config, "osc_svc_hub" ),
            f_oscillator(),
            f_return( RETURN_SUCCESS )
    {
        register_set_handler( "frequency", std::bind( &oscillator_service_hub::handle_set_frequency_request, this, _1 ) );
        register_get_handler( "frequency", std::bind( &oscillator_service_hub::handle_get_frequency_request, this, _1 ) );
        register_set_handler( "amplitude", std::bind( &oscillator_service_hub::handle_set_amplitude_request, this, _1 ) );
        register_get_handler( "amplitude", std::bind( &oscillator_service_hub::handle_get_amplitude_request, this, _1 ) );
        register_get_handler( "in-phase", std::bind( &oscillator_service_hub::handle_get_in_phase_request, this, _1 ) );
        register_get_handler( "quadrature", std::bind( &oscillator_service_hub::handle_get_quadrature_request, this, _1 ) );
        register_get_handler( "iq", std::bind( &oscillator_service_hub::handle_get_iq_request, this, _1 ) );
    }

    oscillator_service_hub::~oscillator_service_hub()
    {
    }

    void oscillator_service_hub::execute()
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

    reply_ptr_t oscillator_service_hub::handle_set_frequency_request( const request_ptr_t a_request )
    {
        f_oscillator.set_frequency( a_request->payload()["values"][0]().as_double() );
        return a_request->reply( dl_success(), "Frequency set" );
    }

    reply_ptr_t oscillator_service_hub::handle_get_frequency_request( const request_ptr_t a_request )
    {
        param_ptr_t t_reply_payload( new param_node() );
        param_node& t_reply_node = t_reply_payload->as_node();
        t_reply_node.add( "value", f_oscillator.get_frequency() );
        return a_request->reply( dl_success(), "Get request succeeded", std::move(t_reply_payload) );
    }


    reply_ptr_t oscillator_service_hub::handle_set_amplitude_request( const request_ptr_t a_request )
    {
        f_oscillator.set_amplitude( a_request->payload()["values"][0]().as_double() );
        return a_request->reply( dl_success(), "Amplitude set" );
    }

    reply_ptr_t oscillator_service_hub::handle_get_amplitude_request( const request_ptr_t a_request )
    {
        param_ptr_t t_reply_payload( new param_node() );
        param_node& t_reply_node = t_reply_payload->as_node();
        t_reply_node.add( "value", f_oscillator.get_amplitude() );
        return a_request->reply( dl_success(), "Get request succeeded", std::move(t_reply_payload) );
    }

/*
    // this could be implemented in a cross-platform way, but it requires more work than I have time for right now, so I'm going to leave it out
    // windows: https://stackoverflow.com/questions/321849/strptime-equivalent-on-windows/321877#321877
    // posix: https://stackoverflow.com/questions/21021388/how-to-parse-a-date-string-into-a-c11-stdchrono-time-point-or-similar
    // the "windows" page has a boost-based cross-platform solution.  current me recommends that since we already use the date_time library

    reply_ptr_t oscillator_service_hub::handle_set_start_time_request( const request_ptr_t a_request )
    {

    }

    reply_ptr_t oscillator_service_hub::handle_get_start_time_request( const request_ptr_t a_request )
    {
        param_ptr_t t_reply_payload( new param_node() );
        param_node& t_reply_node = t_reply_payload->as_node();
        std::time_t t_time = std::chrono::steady_clock::to_time_t( f_oscillator.get_start_time() );
        std::tm t_tm = *std::localtime(&t_time);
        char t_time_arr[100];
        if( std::strftime( t_time_arr, sizeof(t_time_arr), "%c", &t_tm))
        {
            t_reply_node.add( "value", t_time_arr );
            return a_request->reply( dl_success(), "Get request succeeded", std::move(t_reply_payload) );
        }
        return a_request->reply( dl_device_error(), "Time decoding failed" );
    }
*/

    reply_ptr_t oscillator_service_hub::handle_get_in_phase_request( const request_ptr_t a_request )
    {
        param_ptr_t t_reply_payload( new param_node() );
        param_node& t_reply_node = t_reply_payload->as_node();
        t_reply_node.add( "value", f_oscillator.in_phase().second );
        return a_request->reply( dl_success(), "Get request succeeded", std::move(t_reply_payload) );
    }

    reply_ptr_t oscillator_service_hub::handle_get_quadrature_request( const request_ptr_t a_request )
    {
        param_ptr_t t_reply_payload( new param_node() );
        param_node& t_reply_node = t_reply_payload->as_node();
        t_reply_node.add( "value", f_oscillator.quadrature().second );
        return a_request->reply( dl_success(), "Get request succeeded", std::move(t_reply_payload) );
    }

    reply_ptr_t oscillator_service_hub::handle_get_iq_request( const request_ptr_t a_request )
    {
        param_ptr_t t_reply_payload( new param_node() );
        param_node& t_reply_node = t_reply_payload->as_node();
        param_array t_iq_param;
        t_iq_param.push_back( f_oscillator.iq().second.real() );
        t_iq_param.push_back( f_oscillator.iq().second.imag() );
        t_reply_node.add( "value", std::move(t_iq_param) );
        return a_request->reply( dl_success(), "Get request succeeded", std::move(t_reply_payload) );
    }


} /* namespace dripline */
