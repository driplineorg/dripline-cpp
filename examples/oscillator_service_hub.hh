/*
 * oscillator_service_hub.hh
 *
 *  Created on: May 16, 2019
 *      Author: N.S. Oblath
 */

#ifndef EXAMPLES_OSCILLATOR_SERVICE_HUB_HH_
#define EXAMPLES_OSCILLATOR_SERVICE_HUB_HH_

#include "hub.hh"

#include "oscillator.hh"

namespace dripline
{

    class DRIPLINE_EXAMPLES_API oscillator_service_hub : public hub
    {
        public:
            oscillator_service_hub( const scarab::param_node& a_config = scarab::param_node() );
            virtual ~oscillator_service_hub();

            void execute();

            mv_referrable( class oscillator, oscillator );

            mv_accessible( int, return );

        private:
            reply_ptr_t handle_set_frequency_request( const request_ptr_t a_request );
            reply_ptr_t handle_get_frequency_request( const request_ptr_t a_request );

            reply_ptr_t handle_set_amplitude_request( const request_ptr_t a_request );
            reply_ptr_t handle_get_amplitude_request( const request_ptr_t a_request );

            reply_ptr_t handle_set_start_time_request( const request_ptr_t a_request );
            reply_ptr_t handle_get_start_time_request( const request_ptr_t a_request );

            reply_ptr_t handle_get_in_phase_request( const request_ptr_t a_request );
            reply_ptr_t handle_get_quadrature_request( const request_ptr_t a_request );
            reply_ptr_t handle_get_iq_request( const request_ptr_t a_request );
    };

} /* namespace dripline */

#endif /* EXAMPLES_OSCILLATOR_SERVICE_HUB_HH_ */
