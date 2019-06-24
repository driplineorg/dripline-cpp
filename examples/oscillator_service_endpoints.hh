/*
 * oscillator_service_endpoints.hh
 *
 *  Created on: May 20, 2019
 *      Author: N.S. Oblath
 */

#ifndef EXAMPLES_OSCILLATOR_SERVICE_ENDPOINTS_HH_
#define EXAMPLES_OSCILLATOR_SERVICE_ENDPOINTS_HH_

#include "service.hh"

#include "oscillator.hh"

namespace dripline
{
    class DRIPLINE_EXAMPLES_API oscillator_service_endpoints : public service
    {
        public:
            oscillator_service_endpoints( const scarab::param_node& a_config = scarab::param_node() );
            virtual ~oscillator_service_endpoints();

            void execute();

            mv_referrable( class oscillator, oscillator );

            mv_accessible( int, return );
    };


    class DRIPLINE_EXAMPLES_API oscillator_ep : public endpoint
    {
        public:
            oscillator_ep( const std::string& a_name, service_ptr_t a_service );
            virtual ~oscillator_ep();

        protected:
            oscillator_service_endpoints* f_osc_svc;
    };

    class DRIPLINE_EXAMPLES_API oscillator_ep_frequency : public oscillator_ep
    {
        public:
            oscillator_ep_frequency( const std::string& a_name, service_ptr_t a_service );
            virtual ~oscillator_ep_frequency();

            virtual reply_ptr_t do_get_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_set_request( const request_ptr_t a_request );
    };

    class DRIPLINE_EXAMPLES_API oscillator_ep_amplitude : public oscillator_ep
    {
        public:
            oscillator_ep_amplitude( const std::string& a_name, service_ptr_t a_service );
            virtual ~oscillator_ep_amplitude();

            virtual reply_ptr_t do_get_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_set_request( const request_ptr_t a_request );
    };

    class DRIPLINE_EXAMPLES_API oscillator_ep_in_phase : public oscillator_ep
    {
        public:
            oscillator_ep_in_phase( const std::string& a_name, service_ptr_t a_service );
            virtual ~oscillator_ep_in_phase();

            virtual reply_ptr_t do_get_request( const request_ptr_t a_request );
    };

    class DRIPLINE_EXAMPLES_API oscillator_ep_quadrature : public oscillator_ep
    {
        public:
            oscillator_ep_quadrature( const std::string& a_name, service_ptr_t a_service );
            virtual ~oscillator_ep_quadrature();

            virtual reply_ptr_t do_get_request( const request_ptr_t a_request );
    };

    class DRIPLINE_EXAMPLES_API oscillator_ep_iq : public oscillator_ep
    {
        public:
            oscillator_ep_iq( const std::string& a_name, service_ptr_t a_service );
            virtual ~oscillator_ep_iq();

            virtual reply_ptr_t do_get_request( const request_ptr_t a_request );
    };

} /* namespace dripline */

#endif /* EXAMPLES_OSCILLATOR_SERVICE_ENDPOINTS_HH_ */
