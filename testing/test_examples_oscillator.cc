/*
 * test_examples_oscillator.cc
 *
 *  Created on: May 15, 2019
 *      Author: N.S. Oblath
 */

#include "oscillator.hh"

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include <cmath>

#include <iostream>

TEST_CASE( "oscillator", "[examples]" )
{
    using dripline::oscillator;

    oscillator::time_point_t t_start = oscillator::clock_t::now();
    oscillator::time_point_t t_finish = t_start + oscillator::duration_t(1.); // one second later

    oscillator t_osc;
    t_osc.set_amplitude( 5. );
    t_osc.set_frequency( 10. );
    t_osc.set_start_time( t_start );

    std::complex< double > t_expected(
            t_osc.get_amplitude() * cos( t_osc.get_frequency() * oscillator::duration_t(t_finish - t_start).count() ),
            t_osc.get_amplitude() * sin( t_osc.get_frequency() * oscillator::duration_t(t_finish - t_start).count() )
    );

    auto t_iq_return = t_osc.iq( t_finish );
    auto t_i_return = t_osc.in_phase( t_finish );
    auto t_q_return = t_osc.quadrature( t_finish );

    REQUIRE( t_iq_return.first == t_finish );
    REQUIRE( t_i_return.first == t_finish );
    REQUIRE( t_q_return.first == t_finish );

    REQUIRE_THAT( t_iq_return.second.real(), Catch::Matchers::WithinULP(t_expected.real(), 1) );
    REQUIRE_THAT( t_iq_return.second.imag(), Catch::Matchers::WithinULP(t_expected.imag(), 1) );

    REQUIRE_THAT( t_iq_return.second.real(), Catch::Matchers::WithinULP(t_i_return.second, 0) );
    REQUIRE_THAT( t_iq_return.second.imag(), Catch::Matchers::WithinULP(t_q_return.second, 0) );
}

