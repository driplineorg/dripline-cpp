/*
 * oscillator.cc
 *
 *  Created on: May 15, 2019
 *      Author: N.S. Oblath
 */

#include "oscillator.hh"

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

    double oscillator::in_phase( time_point_t a_time )
    {
        return f_amplitude * sin( f_frequency * duration_t( a_time - f_start_time ).count() );
    }

    double oscillator::quadrature( time_point_t a_time )
    {
        return f_amplitude * cos( f_frequency * duration_t( a_time - f_start_time ).count() );
    }

    oscillator::iq_t oscillator::iq( time_point_t a_time )
    {
        return iq_t( in_phase( a_time), quadrature( a_time ) );
    }

} /* namespace dripline */
