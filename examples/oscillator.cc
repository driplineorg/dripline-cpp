/*
 * oscillator.cc
 *
 *  Created on: May 15, 2019
 *      Author: N.S. Oblath
 */

#include "oscillator.hh"

#include <cmath>

namespace dripline
{

    oscillator::oscillator() :
            f_frequency( 1. ),
            f_amplitude( 1. ),
            f_start_time( std::chrono::steady_clock::now() )
    {
    }

    oscillator::~oscillator()
    {
    }

    std::pair< oscillator::time_point_t, double > oscillator::in_phase( time_point_t a_time )
    {
        return std::make_pair( a_time, f_amplitude * cos( f_frequency * duration_t( a_time - f_start_time ).count() ) );
    }

    std::pair< oscillator::time_point_t, double > oscillator::quadrature( time_point_t a_time )
    {
        return std::make_pair( a_time, f_amplitude * sin( f_frequency * duration_t( a_time - f_start_time ).count() ) );
    }

    std::pair< oscillator::time_point_t, oscillator::iq_t > oscillator::iq( time_point_t a_time )
    {
        return std::make_pair( a_time, iq_t( in_phase( a_time).second, quadrature( a_time ).second ) );
    }

} /* namespace dripline */
