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

    LDEBUG( testlog, "Testing basic scheduling" );

    int t_tester = 0;

    t_scheduler.schedule( [&t_tester](){ t_tester = 5; LDEBUG( testlog, "t_tester = " << t_tester ); }, clock_t::now() + std::chrono::seconds(3) );

    REQUIRE( t_tester == 0 );

    LINFO( testlog, "Waiting 4 seconds for events to execute" );
    std::this_thread::sleep_for( std::chrono::seconds(4) );
    LINFO( testlog, "Finished wait" );

    CHECKED_IF( t_tester == 5 )
    {
        // this block only gets executed if the first set of tests passed
        // otherwise it skips and goes on to cancel the thread
        LINFO( testlog, "Testing repeat scheduling" );

        t_scheduler.schedule( [&t_tester](){ t_tester += 5; LDEBUG( testlog, "t_tester = " << t_tester ); }, std::chrono::seconds(3) );
        LINFO( testlog, "Waiting 7 seconds for events to execute" );
        std::this_thread::sleep_for( std::chrono::seconds(7) );
        LINFO( testlog, "Finished wait" );

        CHECK( t_tester == 20 );
    }

    t_scheduler.cancel( 0 );

    t_sched_thread.join();

    REQUIRE( ! t_sched_thread.joinable() );

}



