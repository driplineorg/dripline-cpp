/*
 * test_int_service.cc
 *
 *  Created on: Sept 2, 2021
 *      Author: N.S. Oblath
 */

#include "service.hh"

#include "catch.hpp"

#include <future>

namespace dripline_test
{
    class test_service : public dripline::service
    {
        public:
            test_service( const scarab::param_node& a_config = scarab::param_node(), const std::string& a_queue_name = "",  const std::string& a_broker_address = "", unsigned a_port = 0, const std::string& a_auth_file = "", const bool a_make_connection = true ) :
                    service( a_config, a_queue_name, a_broker_address, a_port, a_auth_file, a_make_connection )
            {}
            ~test_service()
            {}

            bool open_channels()
            {
                return service::open_channels();
            }

            bool setup_queues()
            {
                return service::setup_queues();
            }

            bool bind_keys()
            {
                return service::bind_keys();
            }

            bool start_consuming()
            {
                return service::start_consuming();
            }

            bool stop_consuming()
            {
                return service::stop_consuming();
            }

            bool remove_queue()
            {
                return service::remove_queue();
            }
    };
}


TEST_CASE( "service_basics" )
{
    using dripline_test::test_service;

    test_service t_service( scarab::param_node(), "test_service", "rabbit_broker", 0, "/root/authentication.json" );

    REQUIRE( t_service.start() );

    // async function to cancel the service after a specified amount of time
    auto t_canceler = std::async( std::launch::async, 
            [&t_service](){
                std::cerr << "***** starting sleep" << std::endl;
                std::this_thread::sleep_for( std::chrono::seconds(20) );
                std::cerr << "***** canceling" << std::endl;
                t_service.cancel();
                std::cerr << "***** canceled" << std::endl;
            },
            43
    );

    std::cerr << "^^^^^ starting listen" << std::endl;
    REQUIRE( t_service.listen() );

    REQUIRE( t_service.stop() );
}
