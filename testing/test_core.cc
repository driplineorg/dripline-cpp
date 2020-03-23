/*
 * test_core.cc
 *
 *  Created on: Apr 26, 2019
 *      Author: N.S. Oblath
 */


#include "dripline_exceptions.hh"
#include "core.hh"
#include "return_codes.hh"

#include "catch.hpp"

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
