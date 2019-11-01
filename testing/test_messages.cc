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

TEST_CASE( "message", "[message]" )
{
    std::string t_routing_key( "test.rk" );

    dripline::request_ptr_t t_req_ptr = dripline::msg_request::create(
        scarab::param_ptr_t( new scarab::param_node() ),
        dripline::op_t::cmd,
        t_routing_key );

    // message is valid
    REQUIRE( t_req_ptr );

    LINFO( testlog, *t_req_ptr );

    // header information is set correctly
    REQUIRE( t_req_ptr->get_is_valid() );
    REQUIRE( t_req_ptr->routing_key() == t_routing_key );
    REQUIRE_FALSE( t_req_ptr->correlation_id().empty() );
    REQUIRE( t_req_ptr->get_message_operation() == dripline::op_t::cmd );
    REQUIRE( t_req_ptr->sender_versions().count("dripline") == 1 );
    REQUIRE( t_req_ptr->sender_versions().count("dripline-cpp") == 1 );
    REQUIRE( t_req_ptr->sender_versions().at("dripline-cpp").f_package == "driplineorg/dripline-cpp" );
    REQUIRE( t_req_ptr->message_type() == dripline::msg_t::request );
    REQUIRE( t_req_ptr->parsed_specifier().empty() );

    // conversion of sender info to param_node works
    scarab::param_node t_si_node = t_req_ptr->get_sender_info();
    REQUIRE( t_si_node["exe"]().as_string() == t_req_ptr->sender_exe() );
    REQUIRE( t_si_node["versions"]["dripline"]["version"]().as_string() == t_req_ptr->sender_versions().at("dripline").f_version );
    REQUIRE_FALSE( t_si_node["versions"]["dripline"].as_node().has("commit") );
    REQUIRE_FALSE( t_si_node["versions"]["dripline"].as_node().has("package") );
    REQUIRE( t_si_node["versions"]["dripline-cpp"]["version"]().as_string() == t_req_ptr->sender_versions().at("dripline-cpp").f_version );
    REQUIRE( t_si_node["versions"]["dripline-cpp"]["commit"]().as_string() == t_req_ptr->sender_versions().at("dripline-cpp").f_commit );
    REQUIRE( t_si_node["versions"]["dripline-cpp"]["package"]().as_string() == t_req_ptr->sender_versions().at("dripline-cpp").f_package );
    REQUIRE( t_si_node["hostname"]().as_string() == t_req_ptr->sender_hostname() );
    REQUIRE( t_si_node["username"]().as_string() == t_req_ptr->sender_username() );
    REQUIRE( t_si_node["service_name"]().as_string() == t_req_ptr->sender_service_name() );

    // conversion back from param_node to sender info works
    t_si_node["exe"]().set( "new_exe" );
    t_si_node["versions"]["dripline"]["version"]().set( "v100.0.1" );
    t_si_node["versions"]["dripline-cpp"]["version"]().set( "v300.0.5" );
    t_si_node["versions"]["dripline-cpp"]["commit"]().set( "asdfghjkl" );
    t_si_node["versions"]["dripline-cpp"]["package"]().set( "new-dripline-cpp" );
    t_si_node["hostname"]().set( "new_host" );
    t_si_node["username"]().set( "new_user" );
    t_si_node["service_name"]().set( "new_service" );
    t_req_ptr->set_sender_info( t_si_node );
    REQUIRE( t_si_node["exe"]().as_string() == t_req_ptr->sender_exe() );
    REQUIRE( t_si_node["versions"]["dripline"]["version"]().as_string() == t_req_ptr->sender_versions().at("dripline").f_version );
    REQUIRE( t_si_node["versions"]["dripline-cpp"]["version"]().as_string() == t_req_ptr->sender_versions().at("dripline-cpp").f_version );
    REQUIRE( t_si_node["versions"]["dripline-cpp"]["commit"]().as_string() == t_req_ptr->sender_versions().at("dripline-cpp").f_commit );
    REQUIRE( t_si_node["versions"]["dripline-cpp"]["package"]().as_string() == t_req_ptr->sender_versions().at("dripline-cpp").f_package );
    REQUIRE( t_si_node["hostname"]().as_string() == t_req_ptr->sender_hostname() );
    REQUIRE( t_si_node["username"]().as_string() == t_req_ptr->sender_username() );
    REQUIRE( t_si_node["service_name"]().as_string() == t_req_ptr->sender_service_name() );

    // conversion of the full message to param_node works
    scarab::param_node t_msg_node = t_req_ptr->get_message_param();
    REQUIRE( t_msg_node["routing_key"]().as_string() == t_routing_key );
    REQUIRE( t_msg_node["specifier"]().as_string() == t_req_ptr->parsed_specifier().unparsed() );
    REQUIRE( t_msg_node["correlation_id"]().as_string() == t_req_ptr->correlation_id() );
    REQUIRE( t_msg_node["message_id"]().as_string() == t_req_ptr->message_id() );
    REQUIRE( t_msg_node["reply_to"]().as_string() == t_req_ptr->reply_to() );
    REQUIRE( t_msg_node["message_type"]().as_uint() == to_uint(t_req_ptr->message_type()) );
    REQUIRE( t_msg_node["encoding"]().as_string() == "application/json" );
    REQUIRE( t_msg_node["timestamp"]().as_string() == t_req_ptr->timestamp() );

}


TEST_CASE( "message-conversion", "[message]" )
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
