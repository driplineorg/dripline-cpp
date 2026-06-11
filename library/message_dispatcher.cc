/*
 * message_dispatcher.cc
 *
 *  Created on: Jun 3, 2026
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "message_dispatcher.hh"

#include "dripline_exceptions.hh"

#include "rmqa_consumer.h"
#include "rmqa_topology.h"
#include "rmqa_vhost.h"
#include "rmqp_messageguard.h"

#include "logger.hh"

LOGGER( dlog, "message_dispatcher" );

namespace dripline
{

    message_dispatcher::message_dispatcher() :
            receiver(),
            f_consumer(),
            f_queue()
    {}

    message_dispatcher::message_dispatcher( message_dispatcher&& a_orig ) :
            receiver( std::move(a_orig) ),
            f_consumer( std::move(a_orig.f_consumer) ),
            f_queue( std::move(a_orig.f_queue) )
    {}

    message_dispatcher::~message_dispatcher()
    {}

    message_dispatcher& message_dispatcher::operator=( message_dispatcher&& a_orig )
    {
        receiver::operator=( std::move(a_orig) );
        f_consumer = std::move(a_orig.f_consumer);
        f_queue    = std::move(a_orig.f_queue);
        return *this;
    }

    void message_dispatcher::process_message( message_ptr_t a_message )
    {
        this->submit_message( a_message );
        return;
    }

    void message_dispatcher::start_listening( bsl::shared_ptr< BloombergLP::rmqa::VHost > a_vhost,
                                              BloombergLP::rmqa::Topology& a_topology,
                                              const std::string& a_label )
    {
        using namespace BloombergLP;
        auto t_result = a_vhost->createConsumer(
            a_topology, f_queue,
            [this]( rmqp::MessageGuard& guard ) {
                amqp_envelope_ptr t_envelope = guard.transferOwnership();
                t_envelope->ack();
                handle_message_chunk( std::move(t_envelope) );
            },
            a_label,
            1 );
        if( ! t_result )
        {
            throw connection_error() << "Unable to create consumer: " << t_result.error();
        }
        f_consumer = t_result.value();
    }

    void message_dispatcher::stop_listening()
    {
        if( f_consumer )
        {
            auto t_result = f_consumer->cancelAndDrain();
            if( ! t_result )
            {
                LWARN( dlog, "Error canceling consumer: " << t_result.error() );
            }
            f_consumer.reset();
        }
    }

} /* namespace dripline */
