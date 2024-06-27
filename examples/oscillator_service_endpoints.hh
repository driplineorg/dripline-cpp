/*
 * oscillator_service_endpoints.hh
 *
 *  Created on: May 20, 2019
 *      Author: N.S. Oblath
 *
 *  Comment by the author: this class is a premier example of why it's annoying to make a software-only service
 *  using endpoints rather than hub.  It would be improved by using something like the provider concept.
 *  As it is, however, we have to set the pointers to the service and derived service class in each endpoint
 *  after the service constructor, and this is done in the added function set_pointers().
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
            oscillator_service_endpoints( const scarab::param_node& a_config, const scarab::authentication& a_auth );
            virtual ~oscillator_service_endpoints();

            // annoying pointer initialization that has to be done in this setup _before_ executing
            void set_pointers();

            void execute();

            mv_referrable( class oscillator, oscillator );

            mv_accessible( int, return );
    };


    class DRIPLINE_EXAMPLES_API oscillator_ep : public endpoint
    {
        public:
            oscillator_ep( const std::string& a_name );
            virtual ~oscillator_ep();

        protected:
            friend class oscillator_service_endpoints;
            oscillator_service_endpoints* f_osc_svc;
    };

    class DRIPLINE_EXAMPLES_API oscillator_ep_frequency : public oscillator_ep
    {
        public:
            oscillator_ep_frequency( const std::string& a_name );
            virtual ~oscillator_ep_frequency();

            virtual reply_ptr_t do_get_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_set_request( const request_ptr_t a_request );
    };

    class DRIPLINE_EXAMPLES_API oscillator_ep_amplitude : public oscillator_ep
    {
        public:
            oscillator_ep_amplitude( const std::string& a_name );
            virtual ~oscillator_ep_amplitude();

            virtual reply_ptr_t do_get_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_set_request( const request_ptr_t a_request );
    };

    class DRIPLINE_EXAMPLES_API oscillator_ep_in_phase : public oscillator_ep
    {
        public:
            oscillator_ep_in_phase( const std::string& a_name );
            virtual ~oscillator_ep_in_phase();

            virtual reply_ptr_t do_get_request( const request_ptr_t a_request );
    };

    class DRIPLINE_EXAMPLES_API oscillator_ep_quadrature : public oscillator_ep
    {
        public:
            oscillator_ep_quadrature( const std::string& a_name );
            virtual ~oscillator_ep_quadrature();

            virtual reply_ptr_t do_get_request( const request_ptr_t a_request );
    };

    class DRIPLINE_EXAMPLES_API oscillator_ep_iq : public oscillator_ep
    {
        public:
            oscillator_ep_iq( const std::string& a_name );
            virtual ~oscillator_ep_iq();

            virtual reply_ptr_t do_get_request( const request_ptr_t a_request );
    };

} /* namespace dripline */

#endif /* EXAMPLES_OSCILLATOR_SERVICE_ENDPOINTS_HH_ */
