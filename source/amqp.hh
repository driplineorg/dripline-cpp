/*
Copyright 2015 Noah S. Oblath

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

/*
 * amqp.hh
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

} /* namespace dripline */

#endif /* DRIPLINE_AMPQ_HH_ */
