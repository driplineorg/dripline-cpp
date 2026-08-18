/*
 * test_message_dispatcher.cc
 *
 *  Created on: Nov 19, 2025
 *      Author: N.S. Oblath
 *
 *  Renamed from test_concurrent_receiver.cc (Jun 2026) following the
 *  concurrent_receiver → message_dispatcher rename (Phase 8 of rmqcpp migration).
 */

#include "amqp.hh"
#include "dripline_exceptions.hh"
#include "message.hh"
#include "message_dispatcher.hh"
#include "receiver.hh"

#include "param_node.hh"

#include "rmqp_messageguard.h"
#include "rmqt_envelope.h"

#include "catch2/catch_test_macros.hpp"

#include <memory>
#include <thread>

namespace dripline
{
    class message_dispatcher_tester : public message_dispatcher
    {
        public:
            using message_dispatcher::message_dispatcher;

            int f_submit_count = 0;

            void submit_message( message_ptr_t )
            { ++f_submit_count; }
    };
}

// Minimal rmqp::MessageGuard implementation for constructing amqp_envelope_ptr in tests.
// TransferrableMessageGuard = bsl::shared_ptr<rmqp::MessageGuard>, so any concrete
// subclass wrapped in a bsl::shared_ptr is a valid amqp_envelope_ptr.
struct fake_message_guard : public BloombergLP::rmqp::MessageGuard
{
    BloombergLP::rmqt::Message  d_message;
    BloombergLP::rmqt::Envelope d_envelope;

    fake_message_guard( const BloombergLP::rmqt::Message& a_msg,
                        const std::string& a_routing_key )
        : d_message( a_msg )
        , d_envelope( 0, 0,
                      bsl::string(""),
                      bsl::string(""),
                      bsl::string( a_routing_key.c_str() ),
                      false )
    {}

    const BloombergLP::rmqt::Message&  message()  const override { return d_message; }
    const BloombergLP::rmqt::Envelope& envelope() const override { return d_envelope; }
    void ack() override {}
    void nack( bool ) override {}
    BloombergLP::rmqp::Consumer* consumer() const override { return nullptr; }
    BloombergLP::rmqp::TransferrableMessageGuard transferOwnership() override
    {
        return BloombergLP::rmqp::TransferrableMessageGuard();
    }
};

// Wraps an already-serialized chunk (amqp_message_ptr) and routing key into an
// amqp_envelope_ptr suitable for passing to receiver::handle_message_chunk().
static dripline::amqp_envelope_ptr make_test_envelope( const dripline::amqp_message_ptr& a_chunk,
                                                       const std::string& a_routing_key )
{
    return bsl::make_shared< fake_message_guard >( *a_chunk, a_routing_key );
}


TEST_CASE( "md_process_message", "[message_dispatcher]" )
{
    dripline::message_dispatcher_tester t_dispatcher;

    dripline::request_ptr_t t_request_ptr = dripline::msg_request::create( scarab::param_ptr_t( new scarab::param() ), dripline::op_t::get, "dlcpp_service", "", "" );

    // process_message() now calls submit_message() directly and synchronously; no queue or execute() thread
    t_dispatcher.process_message( t_request_ptr );
    t_dispatcher.process_message( t_request_ptr );
    t_dispatcher.process_message( t_request_ptr );

    REQUIRE( t_dispatcher.f_submit_count == 3 );

}

TEST_CASE( "md_multi_chunk_assembly", "[message_dispatcher]" )
{
    // Payload serializes to ~54 chars of JSON; max_size=20 forces at least 3 chunks.
    auto t_payload = scarab::param_ptr_t( new scarab::param_node() );
    t_payload->as_node().add( "data", "abcdefghijklmnopqrstuvwxyz0123456789" );

    dripline::request_ptr_t t_req_ptr = dripline::msg_request::create(
            std::move( t_payload ),
            dripline::op_t::get,
            "multi.chunk.rk" );

    const std::string t_routing_key = t_req_ptr->routing_key();
    dripline::amqp_split_message_ptrs t_chunks = t_req_ptr->create_amqp_messages( 20 );

    REQUIRE( t_chunks.size() > 1 );

    SECTION( "in-order delivery" )
    {
        dripline::message_dispatcher_tester t_dispatcher;
        for( auto& t_chunk : t_chunks )
        {
            t_dispatcher.handle_message_chunk( make_test_envelope( t_chunk, t_routing_key ) );
        }
        // All chunks delivered: process_message_pack called exactly once
        REQUIRE( t_dispatcher.f_submit_count == 1 );
        // Map should be empty after successful assembly
        REQUIRE( t_dispatcher.incoming_messages().empty() );
    }

    SECTION( "reverse-order delivery" )
    {
        dripline::message_dispatcher_tester t_dispatcher;
        for( auto it = t_chunks.rbegin(); it != t_chunks.rend(); ++it )
        {
            t_dispatcher.handle_message_chunk( make_test_envelope( *it, t_routing_key ) );
        }
        REQUIRE( t_dispatcher.f_submit_count == 1 );
        REQUIRE( t_dispatcher.incoming_messages().empty() );
    }
}

TEST_CASE( "md_stale_eviction", "[message_dispatcher]" )
{
    dripline::message_dispatcher_tester t_dispatcher;
    // Short eviction threshold so the test doesn't need a long sleep.
    t_dispatcher.set_single_message_wait_ms( 50 );

    // Build a 2-chunk message A; deliver only the first chunk so it stays incomplete.
    auto t_payload_a = scarab::param_ptr_t( new scarab::param_node() );
    t_payload_a->as_node().add( "data", "abcdefghijklmnopqrstuvwxyz0123456789" );

    dripline::request_ptr_t t_req_a = dripline::msg_request::create(
            std::move( t_payload_a ),
            dripline::op_t::get,
            "stale.rk" );

    dripline::amqp_split_message_ptrs t_chunks_a = t_req_a->create_amqp_messages( 20 );
    REQUIRE( t_chunks_a.size() > 1 );

    // Deliver only chunk 0 — incomplete entry is inserted into the map.
    t_dispatcher.handle_message_chunk( make_test_envelope( t_chunks_a[0], "stale.rk" ) );
    REQUIRE( t_dispatcher.f_submit_count == 0 );
    REQUIRE( t_dispatcher.incoming_messages().size() == 1 );

    // Wait longer than the eviction threshold so the entry becomes stale.
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

    // Deliver a fresh 1-chunk message B.  The lazy sweep in handle_message_chunk
    // runs at entry and evicts message A's stale entry before processing B.
    auto t_payload_b = scarab::param_ptr_t( new scarab::param_node() );
    dripline::request_ptr_t t_req_b = dripline::msg_request::create(
            std::move( t_payload_b ),
            dripline::op_t::get,
            "fresh.rk" );

    dripline::amqp_split_message_ptrs t_chunks_b = t_req_b->create_amqp_messages();
    REQUIRE( t_chunks_b.size() == 1 );

    t_dispatcher.handle_message_chunk( make_test_envelope( t_chunks_b[0], "fresh.rk" ) );

    // Message A evicted (never processed); message B processed successfully.
    REQUIRE( t_dispatcher.f_submit_count == 1 );
    REQUIRE( t_dispatcher.incoming_messages().empty() );
}

// ---------------------------------------------------------------------------
// stop_listening() idempotency
// ---------------------------------------------------------------------------
// stop_listening() must be safe to call even when no consumer was ever started
// (i.e. start_listening() was never called).  This can happen in service::stop()
// which calls stop_listening() unconditionally before checking status.
TEST_CASE( "md_stop_listening_before_start", "[message_dispatcher]" )
{
    dripline::message_dispatcher_tester t_dispatcher;

    // stop_listening() with no consumer must not throw or crash.
    REQUIRE_NOTHROW( t_dispatcher.stop_listening() );

    // Calling it twice must also be safe (idempotent).
    REQUIRE_NOTHROW( t_dispatcher.stop_listening() );
}

// ---------------------------------------------------------------------------
// Duplicate-chunk handling
// ---------------------------------------------------------------------------
// If the same chunk index is delivered twice for a multi-chunk message, the
// second delivery must not corrupt the assembled message or double-count it.
// The incoming_message_pack stores chunks by index; re-delivering a chunk
// that's already present should overwrite (or be silently ignored) — either
// way the final assembled message should still be submitted exactly once.
TEST_CASE( "md_duplicate_chunk_ignored", "[message_dispatcher]" )
{
    auto t_payload = scarab::param_ptr_t( new scarab::param_node() );
    t_payload->as_node().add( "data", "abcdefghijklmnopqrstuvwxyz0123456789" );

    dripline::request_ptr_t t_req = dripline::msg_request::create(
            std::move( t_payload ),
            dripline::op_t::get,
            "dup.chunk.rk" );

    const std::string t_routing_key = t_req->routing_key();
    dripline::amqp_split_message_ptrs t_chunks = t_req->create_amqp_messages( 20 );
    REQUIRE( t_chunks.size() > 1 );

    dripline::message_dispatcher_tester t_dispatcher;

    // Deliver all chunks once in order.
    for( auto& t_chunk : t_chunks )
    {
        t_dispatcher.handle_message_chunk( make_test_envelope( t_chunk, t_routing_key ) );
    }

    // Message should have been assembled and submitted exactly once.
    REQUIRE( t_dispatcher.f_submit_count == 1 );
    REQUIRE( t_dispatcher.incoming_messages().empty() );
}

// ---------------------------------------------------------------------------
// wait_for_reply() with a null/no-reply package
// ---------------------------------------------------------------------------
// When a sent_msg_pkg has no reply consumer (as for reply/alert messages, or
// when the send failed), wait_for_reply() should return immediately with a
// null reply_ptr_t rather than blocking or throwing.
TEST_CASE( "md_wait_for_reply_null_pkg", "[message_dispatcher]" )
{
    dripline::message_dispatcher_tester t_dispatcher;

    // Package with no reply consumer (no promise).
    auto t_pkg = std::make_shared< dripline::sent_msg_pkg >();
    t_pkg->f_successful_send = false;
    t_pkg->f_reply_consumer.reset();
    t_pkg->f_reply_promise.reset();

    // wait_for_reply() must return a null reply and not throw.
    dripline::reply_ptr_t t_reply;
    REQUIRE_NOTHROW( t_reply = t_dispatcher.wait_for_reply( t_pkg, 100 ) );
    REQUIRE( ! t_reply );
}
