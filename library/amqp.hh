/*
 * amqp.hh
 *
 *  Created on: Jul 13, 2015
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_AMQP_HH_
#define DRIPLINE_AMQP_HH_

#include "dripline_api.hh"

#include "param.hh"

#include "SimpleAmqpClient/SimpleAmqpClient.h"

#include "SimpleAmqpClient/AmqpException.h"
#include "SimpleAmqpClient/AmqpLibraryException.h"


namespace dripline
{
    // convenience typedefs
    typedef AmqpClient::Channel::ptr_t amqp_channel_ptr;
    typedef AmqpClient::Envelope::ptr_t amqp_envelope_ptr;
    typedef AmqpClient::BasicMessage::ptr_t amqp_message_ptr;

    typedef AmqpClient::AmqpException amqp_exception;
    typedef AmqpClient::AmqpLibraryException amqp_lib_exception;

    typedef std::vector< amqp_message_ptr > amqp_split_message_ptrs;

    // conversion functions
    DRIPLINE_API scarab::param_ptr_t table_to_param( const AmqpClient::Table& a_table );
    DRIPLINE_API scarab::param_ptr_t table_to_param( const AmqpClient::Array& a_array );
    DRIPLINE_API scarab::param_ptr_t table_to_param( const AmqpClient::TableValue& a_value );

    DRIPLINE_API AmqpClient::TableValue param_to_table( const scarab::param_node& a_node );
    DRIPLINE_API AmqpClient::TableValue param_to_table( const scarab::param_array& a_array );
    DRIPLINE_API AmqpClient::TableValue param_to_table( const scarab::param_value& a_value );
    DRIPLINE_API AmqpClient::TableValue param_to_table( const scarab::param& a_value );

} /* namespace dripline */

#endif /* DRIPLINE_AMPQ_HH_ */
