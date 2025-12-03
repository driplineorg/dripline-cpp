/*
 * test_concurrent_receiver.cc
 *
 *  Created on: Nov 19, 2025
 *      Author: N.S. Oblath
 */

#include "dripline_exceptions.hh"
#include "message.hh"
#include "receiver.hh"

#include "param_node.hh"

#include "catch2/catch_test_macros.hpp"

#include <chrono>
#include <future>
#include <thread>

namespace dripline
{
    class concurrent_receiver_tester : public concurrent_receiver
    {
        public:
            using concurrent_receiver::concurrent_receiver;

            void submit_message( message_ptr_t )
            {}
    };
}

TEST_CASE( "cr_process_message", "[concurrent_receiver]" )
{
    dripline::concurrent_receiver_tester t_concrecv;

    dripline::request_ptr_t t_request_ptr = dripline::msg_request::create( scarab::param_ptr_t( new scarab::param() ), dripline::op_t::get, "dlcpp_service", "", "" );

    // we process the message before executing the concurrent_receiver.
    // this means the message will be queued.
    t_concrecv.process_message( t_request_ptr );
    t_concrecv.process_message( t_request_ptr );
    t_concrecv.process_message( t_request_ptr );
    t_concrecv.process_message( t_request_ptr );
    t_concrecv.process_message( t_request_ptr );
    t_concrecv.process_message( t_request_ptr );
    t_concrecv.process_message( t_request_ptr );
    t_concrecv.process_message( t_request_ptr );
    t_concrecv.process_message( t_request_ptr );
    t_concrecv.process_message( t_request_ptr );
    REQUIRE( t_concrecv.message_queue().size() == 10 );

    // here we launch the execution asynchronously.
    // we'll give it 1 second to execute, which should be enough, though in principle it's not a 100% guarantee that it'll be done in time.
    // we then cancel the service and move on to verify that the queue is empty.
    auto t_do_execute = [&](){ t_concrecv.concurrent_receiver::execute(); };
    auto t_exe_future = std::async(std::launch::async, t_do_execute);
    std::this_thread::sleep_for( std::chrono::seconds(1) );
    t_concrecv.cancel();
    t_exe_future.wait();

    REQUIRE( t_concrecv.message_queue().empty() );

}




