/*
 * mt_amqp.hh
 *
 *  Created on: Jul 13, 2015
 *      Author: nsoblath
 */

#ifndef DRIPLINE_AMQP_HH_
#define DRIPLINE_AMQP_HH_

#include "SimpleAmqpClient/SimpleAmqpClient.h"

#include "SimpleAmqpClient/AmqpException.h"
#include "SimpleAmqpClient/AmqpLibraryException.h"


namespace dripline
{
    typedef AmqpClient::Channel::ptr_t amqp_channel_ptr;
    typedef AmqpClient::Envelope::ptr_t amqp_envelope_ptr;
    typedef AmqpClient::BasicMessage::ptr_t amqp_message_ptr;

    typedef AmqpClient::AmqpException amqp_exception;
    typedef AmqpClient::AmqpLibraryException amqp_lib_exception;

    typedef std::vector< amqp_message_ptr > amqp_split_message_ptrs;

} /* namespace dripline */

#endif /* DRIPLINE_AMPQ_HH_ */
