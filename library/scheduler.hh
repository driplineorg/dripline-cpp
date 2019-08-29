/*
 * scheduler.hh
 *
 *  Created on: Aug 13, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_SCHEDULER_HH_
#define DRIPLINE_SCHEDULER_HH_

#include "cancelable.hh"

#include "logger.hh"
#include "member_variables.hh"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <utility>

LOGGER( dlog_sh, "scheduler" )

namespace dripline
{
    struct executor
    {
        virtual ~executor() {}
        virtual void operator()( std::function< void() > ) = 0;
    };

    struct simple_executor : executor
    {
        virtual ~simple_executor() {}
        virtual void operator()( std::function< void() > an_executable )
        {
            LDEBUG( dlog_sh, "executing" );
            an_executable();
            return;
        }
    };

    template< typename executor = simple_executor, typename clock = std::chrono::system_clock >
    class scheduler : public scarab::cancelable
    {
        public:
            using clock_t = clock;
            using time_point_t = typename clock::time_point;
            using duration_t = typename clock::duration;
            using executable_t = std::function< void() >;

            scheduler();
            virtual ~scheduler();

            void schedule( time_point_t an_exe_time, executable_t an_executable );

            void execute();

            mv_accessible( duration_t, exe_buffer );
            mv_accessible( duration_t, cycle_time );

            mv_referrable_const( executor, the_executor );

            typedef std::multimap< time_point_t, executable_t > events_map_t;
            mv_referrable_const( events_map_t, events );

        protected:
            std::mutex f_map_mutex;

            std::mutex f_executor_mutex;

            std::condition_variable f_cv;
    };

    template< typename executor, typename clock >
    scheduler< executor, clock >::scheduler() :
            f_exe_buffer( std::chrono::milliseconds(50) ),
            f_cycle_time( std::chrono::milliseconds(500) ),
            f_the_executor(),
            f_events(),
            f_map_mutex(),
            f_executor_mutex(),
            f_cv()
    {}

    template< typename executor, typename clock >
    scheduler< executor, clock >::~scheduler()
    {}

    template< typename executor, typename clock >
    void scheduler< executor, clock >::schedule( time_point_t an_exe_time, executable_t an_executable )
    {
        //time_point_t t_now = clock::now();
        duration_t t_to_submission = an_exe_time - clock::now();
        if( t_to_submission < f_exe_buffer )
        {
            LDEBUG( dlog_sh, "Executing upon submission" );
            std::unique_lock< std::mutex > t_lock( f_executor_mutex );
            f_the_executor( an_executable );
            return;
        }

        bool t_new_first = false;
        std::unique_lock< std::mutex > t_lock( f_map_mutex );
        if( ! f_events.empty() && an_exe_time < f_events.begin()->first )
        {
            LDEBUG( dlog_sh, "New first event" );
            t_new_first = true;
        }
        LDEBUG( dlog_sh, "Inserting new event" );
        f_events.insert( std::make_pair( an_exe_time, an_executable ) );
        if( t_new_first )
        {
            // wake the waiting thread
            LDEBUG( dlog_sh, "That event was first; waking execution thread" );
            f_cv.notify_one();
        }

        return;
    }

    template< typename executor, typename clock >
    void scheduler< executor, clock >::execute()
    {
        LDEBUG( dlog_sh, "Starting scheduler" );
        while( ! is_canceled() )
        {
            std::unique_lock< std::mutex > t_lock( f_map_mutex );
            if( f_events.empty() )
            {
                // wait for f_cycle_time
                f_cv.wait_for( t_lock, f_cycle_time );
                continue;
            }
            else
            {
                auto t_first_event = f_events.begin();
                //time_point_t t_earliest = t_first_exe.first;
                duration_t t_to_earliest = t_first_event->first - clock::now();
                if( t_to_earliest < f_exe_buffer )
                {
                    // do event now
                    LDEBUG( dlog_sh, "Executing first event from the map" );
                    std::unique_lock< std::mutex > t_exe_lock( f_executor_mutex );
                    f_the_executor( t_first_event->second );
                    f_events.erase( t_first_event );
                    continue;
                }
                if( t_to_earliest < f_cycle_time )
                {
                    // wait until t_first_event->first
                    f_cv.wait_until( t_lock, t_first_event->first );
                    continue;
                }
                // wait for f_cycle_time
                f_cv.wait_for( t_lock, f_cycle_time );
                continue;
            }
        }
        LDEBUG( dlog_sh, "Scheduler exiting" );
        return;
    }

} /* namespace dripline */

#endif /* DRIPLINE_SCHEDULER_HH_ */
