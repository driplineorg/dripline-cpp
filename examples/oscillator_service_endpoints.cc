/*
 * oscillator_service_endpoints.cc
 *
 *  Created on: May 20, 2019
 *      Author: N.S. Oblath
 */

#include "oscillator_service_endpoints.hh"

#include "dripline_error.hh"

#include "logger.hh"
#include "signal_handler.hh"

using scarab::param_array;
using scarab::param_node;
using scarab::param_ptr_t;

LOGGER( dlog, "oscillator_service_endpoints" );

namespace dripline
{
    oscillator_ep::oscillator_ep( const std::string& a_name, service& a_service ) :
            endpoint( a_name, a_service ),
            f_osc_svc( static_cast< oscillator_service_endpoints* >( &a_service ) )
    {}

    oscillator_ep::~oscillator_ep()
    {}

    oscillator_ep_frequency::oscillator_ep_frequency( const std::string& a_name, service& a_service ) :
            oscillator_ep( a_name, a_service )
    {}

    oscillator_ep_frequency::~oscillator_ep_frequency()
    {}

    reply_ptr_t oscillator_ep_frequency::do_get_request( const request_ptr_t a_request )
    {
        param_ptr_t t_reply_payload( new param_node() );
        param_node& t_reply_node = t_reply_payload->as_node();
        t_reply_node.add( "value", f_osc_svc->oscillator().get_frequency() );
        return a_request->reply( dl_success(), "Get request succeeded", std::move(t_reply_payload) );
    }

    reply_ptr_t oscillator_ep_frequency::do_set_request( const request_ptr_t a_request )
    {
        f_osc_svc->oscillator().set_frequency( a_request->payload()["values"][0]().as_double() );
        return a_request->reply( dl_success(), "Frequency set" );
    }

    oscillator_ep_amplitude::oscillator_ep_amplitude( const std::string& a_name, service& a_service ) :
            oscillator_ep( a_name, a_service )
    {}

    oscillator_ep_amplitude::~oscillator_ep_amplitude()
    {}

    reply_ptr_t oscillator_ep_amplitude::do_get_request( const request_ptr_t a_request )
    {
        param_ptr_t t_reply_payload( new param_node() );
        param_node& t_reply_node = t_reply_payload->as_node();
        t_reply_node.add( "value", f_osc_svc->oscillator().get_amplitude() );
        return a_request->reply( dl_success(), "Get request succeeded", std::move(t_reply_payload) );
    }

    reply_ptr_t oscillator_ep_amplitude::do_set_request( const request_ptr_t a_request )
    {
        f_osc_svc->oscillator().set_amplitude( a_request->payload()["values"][0]().as_double() );
        return a_request->reply( dl_success(), "Frequency set" );
    }

    oscillator_ep_in_phase::oscillator_ep_in_phase( const std::string& a_name, service& a_service ) :
            oscillator_ep( a_name, a_service )
    {}

    oscillator_ep_in_phase::~oscillator_ep_in_phase()
    {}

    reply_ptr_t oscillator_ep_in_phase::do_get_request( const request_ptr_t a_request )
    {
        param_ptr_t t_reply_payload( new param_node() );
        param_node& t_reply_node = t_reply_payload->as_node();
        t_reply_node.add( "value", f_osc_svc->oscillator().in_phase().second );
        return a_request->reply( dl_success(), "Get request succeeded", std::move(t_reply_payload) );
    }

    oscillator_ep_quadrature::oscillator_ep_quadrature( const std::string& a_name, service& a_service ) :
            oscillator_ep( a_name, a_service )
    {}

    oscillator_ep_quadrature::~oscillator_ep_quadrature()
    {}

    reply_ptr_t oscillator_ep_quadrature::do_get_request( const request_ptr_t a_request )
    {
        param_ptr_t t_reply_payload( new param_node() );
        param_node& t_reply_node = t_reply_payload->as_node();
        t_reply_node.add( "value", f_osc_svc->oscillator().quadrature().second );
        return a_request->reply( dl_success(), "Get request succeeded", std::move(t_reply_payload) );
    }

    oscillator_ep_iq::oscillator_ep_iq( const std::string& a_name, service& a_service ) :
            oscillator_ep( a_name, a_service )
    {}

    oscillator_ep_iq::~oscillator_ep_iq()
    {}

    reply_ptr_t oscillator_ep_iq::do_get_request( const request_ptr_t a_request )
    {
        param_ptr_t t_reply_payload( new param_node() );
        param_node& t_reply_node = t_reply_payload->as_node();
        param_array t_iq_param;
        t_iq_param.push_back( f_osc_svc->oscillator().iq().second.real() );
        t_iq_param.push_back( f_osc_svc->oscillator().iq().second.imag() );
        t_reply_node.add( "value", std::move(t_iq_param) );
        return a_request->reply( dl_success(), "Get request succeeded", std::move(t_reply_payload) );
    }

    oscillator_service_endpoints::oscillator_service_endpoints( const scarab::param_node& a_config ) :
            service( a_config, "osc_svc_ep" ),
            f_oscillator(),
            f_return( RETURN_SUCCESS )
    {
        f_children.insert( std::make_pair( "frequency", std::make_shared< oscillator_ep_frequency >( "frequency", *this ) ) );
        f_children.insert( std::make_pair( "amplitude", std::make_shared< oscillator_ep_amplitude >( "amplitude", *this ) ) );
        f_children.insert( std::make_pair( "in_phase", std::make_shared< oscillator_ep_in_phase >( "in_phase", *this ) ) );
        f_children.insert( std::make_pair( "quadrature", std::make_shared< oscillator_ep_quadrature >( "quadrature", *this ) ) );
        f_children.insert( std::make_pair( "iq", std::make_shared< oscillator_ep_iq >( "iq", *this ) ) );
    }

    oscillator_service_endpoints::~oscillator_service_endpoints()
    {
    }

    void oscillator_service_endpoints::execute()
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
