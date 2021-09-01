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
    };
}

TEST_CASE( "connection" )
{
    dripline_test::test_core t_core( scarab::param_node(), "rabbit_broker", 0, "/root/authentication.json" );

    dripline::amqp_channel_ptr t_channel = t_core.wrap_open_channel();

    REQUIRE( t_channel );
}




