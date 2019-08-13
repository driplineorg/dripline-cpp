/*
 * scheduler.hh
 *
 *  Created on: Aug 13, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_SCHEDULER_HH_
#define DRIPLINE_SCHEDULER_HH_

#include "cancelable.hh"

#include "member_variables.hh"

#include <chrono>
#include <map>
#include <mutex>
#include <utility>

namespace dripline
{
    struct executor
    {
        void operator()( std::function< void > ) = 0;
    };

    struct simple_executor : executor
    {
        void operator()( std::function< void > an_executable )
        {
            an_executable();
            return;
        }
    };

    template< typename executor = simple_executor, typename clock = std::chrono::system_clock >
    class scheduler : public scarab::cancelable
    {
        public:
            using time_point_t = clock::time_point;
            using duration_t = clock::duration;
            using executable_t = std::function< void >;

            scheduler();
            virtual ~scheduler();

            void schedule( time_point_t an_exe_time, executable_t an_executable );

            void execute();

            mv_accessible( duration_t, exe_buffer );
            mv_accessible( duration_t, cycle_time );

            mv_referrable_const( executor, the_executor );

        protected:
            std::map< time_point_t, executable_t > f_executables;
            std::mutex f_map_mutex;

            std::mutex f_executor_mutex;
    };

    template< typename executor, typename clock >
    scheduler< executor, clock >::scheduler() :
            f_exe_buffer( std::chrono::milliseconds(50) ),
            f_cycle_time( std::chrono::milliseconds(500) ),
            f_the_executor(),
            f_executables(),
            f_map_mutex(),
            f_executor_mutex()
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
            std::unique_lock< std::mutex > t_lock( f_executor_mutex );
            f_the_executor( an_executable );
            return;
        }

        bool t_new_first = false;
        std::unique_lock< std::mutex > t_lock( f_map_mutex );
        if( ! f_executables.empty() && an_exe_time < f_executables.begin().first )
        {
            t_new_first = true;
        }
        f_executables.insert( std::make_pair( an_exe_time, an_executable ) );
        if( t_new_first )
        {
            // notify one
        }

        return;
    }

    template< typename executor, typename clock >
    void scheduler< executor, clock >::execute()
    {
        while( ! is_canceled() )
        {
            std::unique_lock< std::mutex > t_lock( f_map_mutex );
            if( f_executables.empty() )
            {
                // wait for f_cycle_time
                continue;
            }
            else
            {
                auto t_first_exe = f_executables.begin();
                //time_point_t t_earliest = t_first_exe.first;
                duration_t t_to_earliest = t_first_exe.first - clock::now();
                if( t_to_earliest < f_exe_buffer )
                {
                    std::unique_lock< std::mutex > t_lock( f_executor_mutex );
                    f_the_executor( t_first_exe.second );
                    f_executables.erase( t_first_exe );
                    continue;
                }
                if( t_to_earliest < f_cycle_time )
                {
                    // wait until t_first_exe.first
                    continue;
                }
                // wait for f_cycle_time
                continue;
            }
        }
        return;
    }

} /* namespace dripline */

#endif /* DRIPLINE_SCHEDULER_HH_ */
