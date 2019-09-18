/*
 * scheduler.hh
 *
 *  Created on: Aug 13, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_SCHEDULER_HH_
#define DRIPLINE_SCHEDULER_HH_

#include "cancelable.hh"

#include "dripline_error.hh"

#include "logger.hh"
#include "member_variables.hh"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <thread>
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
    class scheduler : virtual public scarab::cancelable
    {
        public:
            using clock_t = clock;
            using time_point_t = typename clock::time_point;
            using duration_t = typename clock::duration;
            using executable_t = std::function< void() >;

            struct event
            {
                executable_t f_executable;
                int f_id;
            };
            typedef std::multimap< time_point_t, event > events_map_t;

            scheduler();
            virtual ~scheduler();

            int schedule( executable_t an_executable, time_point_t an_exe_time );
            int schedule( executable_t an_executable, duration_t an_interval, time_point_t an_exe_time = clock::now() );

            void unschedule( int an_id );

            void execute();

            mv_accessible( duration_t, exe_buffer );
            mv_accessible( duration_t, cycle_time );

            mv_referrable_const( executor, the_executor );

            mv_referrable_const( events_map_t, events );

            mv_accessible_static( int, curr_id )

        protected:
            void schedule_repeating( executable_t an_executable, duration_t an_interval, int an_id, time_point_t a_rep_start = clock::now() );

            std::recursive_mutex f_scheduler_mutex;  // recursive_mutex is used so that the mutex can be locked twice by the same thread when using a repeating schedule

            std::mutex f_executor_mutex;

            std::condition_variable_any f_cv;
            std::thread f_scheduler_thread;
    };

    template< typename executor, typename clock >
    int scheduler< executor, clock >::s_curr_id = 0;

    template< typename executor, typename clock >
    scheduler< executor, clock >::scheduler() :
            f_exe_buffer( std::chrono::milliseconds(50) ),
            f_cycle_time( std::chrono::milliseconds(500) ),
            f_the_executor(),
            f_events(),
            f_scheduler_mutex(),
            f_executor_mutex(),
            f_cv(),
            f_scheduler_thread()
    {}

    template< typename executor, typename clock >
    scheduler< executor, clock >::~scheduler()
    {}

    template< typename executor, typename clock >
    int scheduler< executor, clock >::schedule( executable_t an_executable, time_point_t an_exe_time )
    {
        bool t_new_first = false;
        std::unique_lock< std::recursive_mutex > t_lock( f_scheduler_mutex );
        if( f_events.empty() || an_exe_time < f_events.begin()->first )
        {
            LDEBUG( dlog_sh, "New first event" );
            t_new_first = true;
        }
        LDEBUG( dlog_sh, "Inserting new event" );
        event t_event;
        t_event.f_executable = an_executable;
        t_event.f_id = s_curr_id++;
        f_events.insert( std::make_pair( an_exe_time, t_event ) );
        if( t_new_first )
        {
            // wake the waiting thread
            LDEBUG( dlog_sh, "That event was first; waking execution thread" );
            f_cv.notify_one();
        }

        return t_event.f_id;
    }

    template< typename executor, typename clock >
    int scheduler< executor, clock >::schedule( executable_t an_executable, duration_t an_interval, time_point_t an_exe_time )
    {
        // if the interval is too short, it's more likely that the execution time will be longer than the interval
        if( an_interval < 2*f_exe_buffer )
        {
            throw dripline_error() << "Cannot schedule executions with an interval of less than " <<  std::chrono::duration_cast<std::chrono::seconds>(2*f_exe_buffer).count() << " seconds";
        }

        std::unique_lock< std::recursive_mutex > t_lock( f_scheduler_mutex );
        int t_id = scheduler< executor, clock >::s_curr_id++;
        schedule_repeating( an_executable, an_interval, t_id, an_exe_time );

        // return the id
        return t_id;
    }

    template< typename executor, typename clock >
    void scheduler< executor, clock >::schedule_repeating( executable_t an_executable, duration_t an_interval, int an_id, time_point_t a_rep_start )
    {
        LDEBUG( dlog_sh, "Scheduling a repeating event" );

        // create the wrapper executable around the event
        executable_t t_wrapped_executable = [this, an_executable, an_interval, an_id, a_rep_start](){ 
            LDEBUG( dlog_sh, "wrapped execution" );
            // reschedule itself an_interval in the future
            this->schedule_repeating( an_executable, an_interval, an_id, a_rep_start + an_interval );
            // execute the event
            LDEBUG( dlog_sh, "executing the wrapped executable" );
            an_executable();
        };

        // create the event
        event t_event;
        t_event.f_executable = t_wrapped_executable;
        t_event.f_id = an_id;

        // check if this'll be a new first event
        bool t_new_first = false;
        std::unique_lock< std::recursive_mutex > t_lock( f_scheduler_mutex );
        if( f_events.empty() || a_rep_start < f_events.begin()->first )
        {
            LDEBUG( dlog_sh, "New first event" );
            t_new_first = true;
        }

        // add the event to the map
        f_events.insert( std::make_pair( a_rep_start, t_event ) );
        if( t_new_first )
        {
            // wake the waiting thread
            LDEBUG( dlog_sh, "That event was first; waking execution thread" );
            f_cv.notify_one();
        }

        return;
    }

    template< typename executor, typename clock >
    void scheduler< executor, clock >::unschedule( int an_id )
    {
        std::unique_lock< std::recursive_mutex > t_lock( f_scheduler_mutex );
        auto i_event = f_events.begin();
        for( ; i_event->second.f_id != an_id && i_event != f_events.end(); ++i_event ) {
            LDEBUG( dlog_sh, "Looking for event with id <" << an_id << ">; found one with id <" << i_event->second.f_id << ">" );
        }

        if( i_event->second.f_id == an_id ) 
        {
            LDEBUG( dlog_sh, "Found event with id <" << i_event->second.f_id << ">; erasing it now" );
            f_events.erase( i_event );
            LDEBUG( dlog_sh, "Removed event <" << an_id << "> from the schedule" );
        }
        else
        {
            LDEBUG( dlog_sh, "No event with id <" << an_id << "> found" );
        }
        
        return;
    }

    template< typename executor, typename clock >
    void scheduler< executor, clock >::execute()
    {
        LDEBUG( dlog_sh, "Starting scheduler" );
        while( ! is_canceled() )
        {
            std::unique_lock< std::recursive_mutex > t_lock( f_scheduler_mutex );
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
                    f_the_executor( t_first_event->second.f_executable );
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
