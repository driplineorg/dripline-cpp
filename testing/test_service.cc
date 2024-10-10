/*
 * test_service.cc
 *
 *  Created on: Jun 10, 2021
 *      Author: N.S. Oblath
 */

#include "dripline_exceptions.hh"
#include "service.hh"

#include "authentication.hh"
#include "param_node.hh"

#include "catch2/catch_test_macros.hpp"

#include <chrono>
#include <future>
#include <thread>

TEST_CASE( "process_message", "[service]" )
{
    dripline::service t_service( scarab::param_node(), scarab::authentication(), false);

    dripline::request_ptr_t t_request_ptr = dripline::msg_request::create( scarab::param_ptr_t( new scarab::param() ), dripline::op_t::get, "dlcpp_service", "", "" );

    // we process the message before executing the concurrent_receiver.
    // this means the message will be queued.
    t_service.process_message( t_request_ptr );
    REQUIRE( t_service.message_queue().size() == 1 );

    // here we launch the execution asynchronously.
    // we'll give it 1 second to execute, which should be enough, though in principle it's not a 100% guarantee that it'll be done in time.
    // we then cancel the service and move on to verify that the queue is empty.
    auto t_do_execute = [&](){ t_service.concurrent_receiver::execute(); };
    auto t_exe_future = std::async(std::launch::async, t_do_execute);
    std::this_thread::sleep_for( std::chrono::seconds(1) );
    t_service.cancel();
    t_exe_future.wait();

    REQUIRE( t_service.message_queue().empty() );

}




