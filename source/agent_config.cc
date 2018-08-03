/*
Copyright 2016 Noah S. Oblath

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
 * agent_config.cc
 *
 *  Created on: Jun 2, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "agent_config.hh"

#include "macros.hh"

using std::string;
using scarab::param_value;

namespace dripline
{

    agent_config::agent_config()
    {
        // default agent configuration

        param_node t_amqp_node;
        t_amqp_node.add( "broker-port", 5672 );
        t_amqp_node.add( "broker", "localhost" );
        t_amqp_node.add( "reply-timeout-ms", 10000 );
#ifdef DRIPLINE_AUTH_FILE
        t_amqp_node.add( "auth-file", TOSTRING( DRIPLINE_AUTH_FILE ) );
#endif
        add( "amqp", std::move(t_amqp_node) );
    }

    agent_config::~agent_config()
    {
    }

} /* namespace dripline */
