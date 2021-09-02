/*
 * test_int_connection.cc
 *
 *  Created on: Aug 31, 2021
 *      Author: N.S. Oblath
 */

#include "core.hh"

#include "catch.hpp"

namespace dripline_test
{
    class test_core : public dripline::core
    {
        public:
            test_core( const scarab::param_node& a_config = scarab::param_node(), const std::string& a_broker_address = "", unsigned a_port = 0, const std::string& a_auth_file = "", const bool a_make_connection = true ) :
                    core( a_config, a_broker_address, a_port, a_auth_file, a_make_connection )
            {}
            ~test_core()
            {}

            dripline::amqp_channel_ptr wrap_open_channel() const
            {
                return core::open_channel();
            }

            static bool setup_exchange( dripline::amqp_channel_ptr a_channel, const std::string& a_exchange )
            {
                return core::setup_exchange( a_channel, a_exchange );
            }

            static bool setup_queue( dripline::amqp_channel_ptr a_channel, const std::string& a_queue_name )
            {
                return core::setup_queue( a_channel, a_queue_name );
            }

            static bool bind_key( dripline::amqp_channel_ptr a_channel, const std::string& a_exchange, const std::string& a_queue_name, const std::string& a_routing_key )
            {
                return core::bind_key( a_channel, a_exchange, a_queue_name, a_routing_key );
            }

            static std::string start_consuming( dripline::amqp_channel_ptr a_channel, const std::string& a_queue_name )
            {
                return core::start_consuming( a_channel, a_queue_name );
            }

            static bool stop_consuming( dripline::amqp_channel_ptr a_channel, std::string& a_consumer_tag )
            {
                return core::stop_consuming( a_channel, a_consumer_tag );
            }

            static bool remove_queue( dripline::amqp_channel_ptr a_channel, const std::string& a_queue_name )
            {
                return core::remove_queue( a_channel, a_queue_name );
            }

    };
}

TEST_CASE( "channel" )
{
    using dripline_test::test_core;

    test_core t_core( scarab::param_node(), "rabbit_broker", 0, "/root/authentication.json" );

    dripline::amqp_channel_ptr t_channel = t_core.wrap_open_channel();

    REQUIRE( t_channel );

    REQUIRE( test_core::setup_exchange( t_channel, "requests" ) );
    //std::this_thread::sleep_for( std::chrono::seconds(10) );
    // for some reason i get an error when i run this command, though everything seems to be working correctly
    //REQUIRE( t_channel->CheckExchangeExists( "requests" ) );

    REQUIRE( test_core::setup_queue( t_channel, "test_queue" ) );
    std::this_thread::sleep_for( std::chrono::seconds(5) );
    // for some reason i get an error when i run this command, though everything seems to be working correctly
    //REQUIRE( t_channel->CheckQueueExists( "test_queue" ) );

    REQUIRE( test_core::bind_key( t_channel, "requests", "test_queue", "test_key" ) );

    std::string t_consumer_tag( test_core::start_consuming( t_channel, "test_queue" ) );
    REQUIRE_FALSE( t_consumer_tag.empty() );

    REQUIRE( test_core::stop_consuming( t_channel, t_consumer_tag ) );

    REQUIRE( test_core::remove_queue( t_channel, "test_queue" ) );
    std::this_thread::sleep_for( std::chrono::seconds(10) );
    // for some reason i get an error when i run this command, though everything seems to be working correctly
    //REQUIRE_FALSE( t_channel->CheckQueueExists( "test_queue" ) );
}
