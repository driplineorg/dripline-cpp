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

#include "rmqt_fieldvalue.h"
#include "rmqt_message.h"
#include "rmqp_messageguard.h"
#include "bsl_memory.h"


namespace dripline
{
    // convenience typedefs
    typedef BloombergLP::rmqp::TransferrableMessageGuard amqp_envelope_ptr;
    typedef bsl::shared_ptr< BloombergLP::rmqt::Message > amqp_message_ptr;

    typedef std::vector< amqp_message_ptr > amqp_split_message_ptrs;

    // conversion functions
    DRIPLINE_API scarab::param_ptr_t table_to_param( const BloombergLP::rmqt::FieldTable& a_table );
    DRIPLINE_API scarab::param_ptr_t table_to_param( const BloombergLP::rmqt::FieldArray& a_array );
    DRIPLINE_API scarab::param_ptr_t table_to_param( const BloombergLP::rmqt::FieldValue& a_value );

    DRIPLINE_API BloombergLP::rmqt::FieldValue param_to_table( const scarab::param_node& a_node );
    DRIPLINE_API BloombergLP::rmqt::FieldValue param_to_table( const scarab::param_array& a_array );
    DRIPLINE_API BloombergLP::rmqt::FieldValue param_to_table( const scarab::param_value& a_value );
    DRIPLINE_API BloombergLP::rmqt::FieldValue param_to_table( const scarab::param& a_value );

} /* namespace dripline */

#endif /* DRIPLINE_AMPQ_HH_ */
