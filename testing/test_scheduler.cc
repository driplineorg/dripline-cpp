/*
 * test_scheduler.cc
 *
 *  Created on: Aug 14, 2019
 *      Author: N.S. Oblath
 */

#include "scheduler.hh"

#include "logger.hh"

#include "catch.hpp"

#include <thread>

LOGGER( testlog, "test_scheduler" );

TEST_CASE( "scheduler", "[utility]" )
{
    using clock_t = typename dripline::scheduler<>::clock_t;

    dripline::scheduler<> t_scheduler;

    std::thread t_sched_thread( &dripline::scheduler<>::execute, &t_scheduler );

    REQUIRE( t_sched_thread.joinable() );
    REQUIRE( t_scheduler.events().empty() );

    int t_tester = 0;

    t_scheduler.schedule( clock_t::now() + std::chrono::seconds(3), [&t_tester](){ t_tester = 5; } );

    REQUIRE( t_tester == 0 );

    LINFO( testlog, "Waiting for events to execute" );
    std::this_thread::sleep_for( std::chrono::seconds(4) );

    REQUIRE( t_tester == 5 );

    t_scheduler.cancel( 0 );

    t_sched_thread.join();

    REQUIRE( ! t_sched_thread.joinable() );

}



