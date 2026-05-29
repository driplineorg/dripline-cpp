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

namespace dripline
{
    class concurrent_receiver_tester : public concurrent_receiver
    {
        public:
            using concurrent_receiver::concurrent_receiver;

            int f_submit_count = 0;

            void submit_message( message_ptr_t )
            { ++f_submit_count; }
    };
}

TEST_CASE( "cr_process_message", "[concurrent_receiver]" )
{
    dripline::concurrent_receiver_tester t_concrecv;

    dripline::request_ptr_t t_request_ptr = dripline::msg_request::create( scarab::param_ptr_t( new scarab::param() ), dripline::op_t::get, "dlcpp_service", "", "" );

    // process_message() now calls submit_message() directly and synchronously; no queue or execute() thread
    t_concrecv.process_message( t_request_ptr );
    t_concrecv.process_message( t_request_ptr );
    t_concrecv.process_message( t_request_ptr );

    REQUIRE( t_concrecv.f_submit_count == 3 );

}




