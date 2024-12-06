/*
 * oscillator_service_endpoints.cc
 *
 *  Created on: May 20, 2019
 *      Author: N.S. Oblath
 */

#include "oscillator_service_endpoints.hh"

#include "dripline_exceptions.hh"

#include "logger.hh"
#include "signal_handler.hh"

using scarab::param_array;
using scarab::param_node;
using scarab::param_ptr_t;

LOGGER( dlog, "oscillator_service_endpoints" );

namespace dripline
{
    oscillator_ep::oscillator_ep( const std::string& a_name ) :
            endpoint( a_name ),
            f_osc_svc()
    {}

    oscillator_ep::~oscillator_ep()
    {}

    oscillator_ep_frequency::oscillator_ep_frequency( const std::string& a_name ) :
            oscillator_ep( a_name )
    {}

    oscillator_ep_frequency::~oscillator_ep_frequency()
    {}

    reply_ptr_t oscillator_ep_frequency::do_get_request( const request_ptr_t a_request )
    {
        param_ptr_t t_reply_payload( new param_node() );
        param_node& t_reply_node = t_reply_payload->as_node();
        t_reply_node.add( "value", static_cast< oscillator_service_endpoints* >( f_service )->oscillator().get_frequency() );
        return a_request->reply( dl_success(), "Get request succeeded", std::move(t_reply_payload) );
    }

    reply_ptr_t oscillator_ep_frequency::do_set_request( const request_ptr_t a_request )
    {
        static_cast< oscillator_service_endpoints* >( f_service )->oscillator().set_frequency( a_request->payload()["values"][0]().as_double() );
        return a_request->reply( dl_success(), "Frequency set" );
    }

    oscillator_ep_amplitude::oscillator_ep_amplitude( const std::string& a_name ) :
            oscillator_ep( a_name )
    {}

    oscillator_ep_amplitude::~oscillator_ep_amplitude()
    {}

    reply_ptr_t oscillator_ep_amplitude::do_get_request( const request_ptr_t a_request )
    {
        param_ptr_t t_reply_payload( new param_node() );
        param_node& t_reply_node = t_reply_payload->as_node();
        t_reply_node.add( "value", static_cast< oscillator_service_endpoints* >( f_service )->oscillator().get_amplitude() );
        return a_request->reply( dl_success(), "Get request succeeded", std::move(t_reply_payload) );
    }

    reply_ptr_t oscillator_ep_amplitude::do_set_request( const request_ptr_t a_request )
    {
        static_cast< oscillator_service_endpoints* >( f_service )->oscillator().set_amplitude( a_request->payload()["values"][0]().as_double() );
        return a_request->reply( dl_success(), "Frequency set" );
    }

    oscillator_ep_in_phase::oscillator_ep_in_phase( const std::string& a_name ) :
            oscillator_ep( a_name )
    {}

    oscillator_ep_in_phase::~oscillator_ep_in_phase()
    {}

    reply_ptr_t oscillator_ep_in_phase::do_get_request( const request_ptr_t a_request )
    {
        param_ptr_t t_reply_payload( new param_node() );
        param_node& t_reply_node = t_reply_payload->as_node();
        t_reply_node.add( "value", static_cast< oscillator_service_endpoints* >( f_service )->oscillator().in_phase().second );
        return a_request->reply( dl_success(), "Get request succeeded", std::move(t_reply_payload) );
    }

    oscillator_ep_quadrature::oscillator_ep_quadrature( const std::string& a_name ) :
            oscillator_ep( a_name )
    {}

    oscillator_ep_quadrature::~oscillator_ep_quadrature()
    {}

    reply_ptr_t oscillator_ep_quadrature::do_get_request( const request_ptr_t a_request )
    {
        param_ptr_t t_reply_payload( new param_node() );
        param_node& t_reply_node = t_reply_payload->as_node();
        t_reply_node.add( "value", static_cast< oscillator_service_endpoints* >( f_service )->oscillator().quadrature().second );
        return a_request->reply( dl_success(), "Get request succeeded", std::move(t_reply_payload) );
    }

    oscillator_ep_iq::oscillator_ep_iq( const std::string& a_name ) :
            oscillator_ep( a_name )
    {}

    oscillator_ep_iq::~oscillator_ep_iq()
    {}

    reply_ptr_t oscillator_ep_iq::do_get_request( const request_ptr_t a_request )
    {
        param_ptr_t t_reply_payload( new param_node() );
        param_node& t_reply_node = t_reply_payload->as_node();
        param_array t_iq_param;
        t_iq_param.push_back( static_cast< oscillator_service_endpoints* >( f_service )->oscillator().iq().second.real() );
        t_iq_param.push_back( static_cast< oscillator_service_endpoints* >( f_service )->oscillator().iq().second.imag() );
        t_reply_node.add( "value", std::move(t_iq_param) );
        return a_request->reply( dl_success(), "Get request succeeded", std::move(t_reply_payload) );
    }

    oscillator_service_endpoints::oscillator_service_endpoints( const scarab::param_node& a_config, const scarab::authentication& a_auth ) :
            scarab::cancelable(),
            service( a_config, a_auth ),
            f_oscillator(),
            f_return( dl_success().rc_value() )
    {
        add_child( std::make_shared< oscillator_ep_frequency >( "frequency" ) );
        add_child( std::make_shared< oscillator_ep_amplitude >( "amplitude" ) );
        add_async_child( std::make_shared< oscillator_ep_in_phase >( "in_phase" ) );
        add_async_child( std::make_shared< oscillator_ep_quadrature >( "quadrature" ) );
        add_async_child( std::make_shared< oscillator_ep_iq >( "iq" ) );
    }

    oscillator_service_endpoints::~oscillator_service_endpoints()
    {
    }

    void oscillator_service_endpoints::execute()
    {
        auto t_cwrap = scarab::wrap_cancelable( *this );
        scarab::signal_handler::add_cancelable( t_cwrap );

        try
        {
            run();
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Exception caught: " << e.what() );
            LERROR( dlog, "Exiting service" );
            f_return = dl_service_error().rc_value() / 100;
            scarab::signal_handler::cancel_all( f_return );
        }

        if( scarab::signal_handler::get_exited() )
        {
            f_return = scarab::signal_handler::get_return_code();
        }

        return;
    }

} /* namespace dripline */
