/*
 * test_messages.cc
 *
 *  Created on: Jan 8, 2019
 *      Author: N.S. Oblath
 */

#include "message.hh"

#include "logger.hh"

#include "catch.hpp"

LOGGER( testlog, "test_message" );

TEST_CASE( "conversion", "[message]" )
{
    SECTION( "request" )
    {
        dripline::request_ptr_t t_req_ptr = dripline::msg_request::create(
                scarab::param_ptr_t( new scarab::param_node() ),
                dripline::op_t::cmd,
                "test.rk" );
        //LDEBUG( testlog, "Input request: " << *t_req_ptr );

        REQUIRE( t_req_ptr );

        dripline::amqp_split_message_ptrs t_amqp_msg_ptrs = t_req_ptr->create_amqp_messages();

        REQUIRE( t_amqp_msg_ptrs[0] );
        REQUIRE( t_amqp_msg_ptrs.size() == 1 );

        dripline::message_ptr_t t_conv_msg_ptr = dripline::message::process_message( t_amqp_msg_ptrs, "test.rk" );

        REQUIRE( t_conv_msg_ptr->is_request() );

        dripline::request_ptr_t t_conv_req_ptr = std::static_pointer_cast< dripline::msg_request >(t_conv_msg_ptr);
        //LDEBUG( testlog, "Output request: " << *t_conv_req_ptr );

        REQUIRE( *t_req_ptr == *t_conv_req_ptr );
    }

    SECTION( "reply" )
    {
        dripline::reply_ptr_t t_rep_ptr = dripline::msg_reply::create(
                0,
                "Success!",
                scarab::param_ptr_t( new scarab::param_node() ),
                "test.rk" );
        //LDEBUG( testlog, "Input reply: " << *t_rep_ptr );

        REQUIRE( t_rep_ptr );

        dripline::amqp_split_message_ptrs t_amqp_msg_ptrs = t_rep_ptr->create_amqp_messages();

        REQUIRE( t_amqp_msg_ptrs[0] );
        REQUIRE( t_amqp_msg_ptrs.size() == 1 );

        dripline::message_ptr_t t_conv_msg_ptr = dripline::message::process_message( t_amqp_msg_ptrs, "test.rk" );

        REQUIRE( t_conv_msg_ptr->is_reply() );

        dripline::reply_ptr_t t_conv_rep_ptr = std::static_pointer_cast< dripline::msg_reply >(t_conv_msg_ptr);
        //LDEBUG( testlog, "Output request: " << *t_conv_rep_ptr );

        REQUIRE( *t_rep_ptr == *t_conv_rep_ptr );
    }

    SECTION( "alert" )
    {
        dripline::alert_ptr_t t_alert_ptr = dripline::msg_alert::create(
                scarab::param_ptr_t( new scarab::param_node() ),
                "test.rk" );
        //LDEBUG( testlog, "Input request: " << *t_alert_ptr );

        REQUIRE( t_alert_ptr );

        dripline::amqp_split_message_ptrs t_amqp_msg_ptrs = t_alert_ptr->create_amqp_messages();

        REQUIRE( t_amqp_msg_ptrs[0] );
        REQUIRE( t_amqp_msg_ptrs.size() == 1 );

        dripline::message_ptr_t t_conv_msg_ptr = dripline::message::process_message( t_amqp_msg_ptrs, "test.rk" );

        REQUIRE( t_conv_msg_ptr->is_alert() );

        dripline::alert_ptr_t t_conv_alert_ptr = std::static_pointer_cast< dripline::msg_alert >(t_conv_msg_ptr);
        //LDEBUG( testlog, "Output request: " << *t_conv_alert_ptr );

        REQUIRE( *t_alert_ptr == *t_conv_alert_ptr );
    }


}
