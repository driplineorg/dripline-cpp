/*
 * oscillator.hh
 *
 *  Created on: May 15, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_EXAMPLES_OSCILLATOR_HH_
#define DRIPLINE_EXAMPLES_OSCILLATOR_HH_

#include "dripline_api.hh"

#include "member_variables.hh"

#include <chrono>
#include <complex>
#include <utility>

namespace dripline
{

    class DRIPLINE_EXAMPLES_API oscillator
    {
        public:
            using clock_t = std::chrono::steady_clock;
            using duration_t = std::chrono::duration< double >;
            using time_point_t = std::chrono::time_point< std::chrono::steady_clock, duration_t >;

            using iq_t = std::complex< double >;

        public:
            oscillator();
            virtual ~oscillator();

            /// Oscillator frequency in Hz
            mv_accessible( double, frequency );

            /// Amplitude of the oscillations
            mv_accessible( double, amplitude );

            /// Start time (defaults to current time the oscillator is created)
            mv_accessible( time_point_t, start_time );

        public:
            std::pair< time_point_t, double > in_phase( time_point_t a_time = clock_t::now() );
            std::pair< time_point_t, double > quadrature( time_point_t a_time = clock_t::now() );
            std::pair< time_point_t, iq_t > iq( time_point_t a_time = clock_t::now() );

    };

} /* namespace dripline */

#endif /* DRIPLINE_EXAMPLES_OSCILLATOR_HH_ */
