/*
 * test_core.cc
 *
 *  Created on: Apr 26, 2019
 *      Author: N.S. Oblath
 */


#include "core.hh"
#include "dripline_exceptions.hh"
#include "return_codes.hh"

#include "param_codec.hh"

#include "catch.hpp"

#include <boost/filesystem.hpp>


TEST_CASE( "send_offline", "[core]" )
{
    using scarab::param_ptr_t;
    using scarab::param;

    dripline::core::s_offline = true;

    dripline::core t_core( true );

    dripline::alert_ptr_t t_alert_ptr = dripline::msg_alert::create( param_ptr_t(new param()), "" );

    REQUIRE_THROWS_AS( t_core.send( t_alert_ptr ), dripline::message_ptr_t );

    dripline::request_ptr_t t_request_ptr = dripline::msg_request::create( param_ptr_t( new param() ), dripline::op_t::cmd, "routing.key", "specifier", "" );

    REQUIRE_THROWS_AS( t_core.send( t_request_ptr ), dripline::message_ptr_t );

    dripline::reply_ptr_t t_reply_ptr = dripline::msg_reply::create( dripline::dl_success(), "reply", param_ptr_t( new param() ), "routing.key" );

    REQUIRE_THROWS_AS( t_core.send( t_reply_ptr ), dripline::message_ptr_t );
}

TEST_CASE( "config_retcode", "[core]" )
{
    using scarab::param_ptr_t;
    using scarab::param_node;
    using scarab::param_array;

    param_node t_code;
    t_code.add( "name", "test_code" );
    t_code.add( "value", 5000 );
    t_code.add( "description", "test code" );

    param_array t_ret_codes;
    t_ret_codes.push_back( t_code );

    param_ptr_t t_param( new param_node() );
    param_node& t_config = t_param->as_node();
    t_config.add( "return-codes", t_ret_codes );

    auto t_factory = scarab::indexed_factory< unsigned, dripline::return_code >::get_instance();

    // test adding a return code through a config
    dripline::core t_core( t_config );
    REQUIRE( t_factory->has_class( 5000 ) );

    t_config["return-codes"][0]["value"]() = 5001;
    t_config["return-codes"][0].as_node().erase("description");

    // test adding a return code with an invalid config
    REQUIRE_THROWS_AS( dripline::core( t_config ), dripline::dripline_error );
}

TEST_CASE( "config_retcode_fromfile", "[core]" )
{
    using scarab::param_ptr_t;
    using scarab::param_node;
    using scarab::param_array;

    param_node t_code;
    t_code.add( "name", "test_code_fromfile" );
    t_code.add( "value", 5100 );
    t_code.add( "description", "test code from file" );

    param_array t_ret_codes;
    t_ret_codes.push_back( t_code );

    // write the temporary file with the array of retcodes
    scarab::param_translator t_translator;
    std::string t_temp_filename( boost::filesystem::unique_path().native() );
    t_temp_filename += ".yaml";
    t_translator.write_file( t_ret_codes, t_temp_filename );

    // the config object
    param_ptr_t t_param( new param_node() );
    param_node& t_config = t_param->as_node();
    t_config.add( "return-codes", t_temp_filename );

    auto t_factory = scarab::indexed_factory< unsigned, dripline::return_code >::get_instance();

    // test adding a return code through a config
    dripline::core t_core( t_config );
    REQUIRE( t_factory->has_class( 5100 ) );

    std::remove( t_temp_filename.c_str() );

}
