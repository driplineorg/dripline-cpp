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
 * agent.hh
 *
 *  Created on: Jun 2, 2016
 *      Author: nsoblath
 *
 *  Expected configuration supplied to the constructor:
 *
 *  {
 *      "[operation: run, get, set, or cmd] : "",
 *      "amqp" : {
 *          "broker" : "[address]",
 *          "broker-port" : [port],
 *          "exchange" : "[exchange]",
 *          "auth-file" : "[authentication file]",  // optional; must live in the user's home directory
 *          "reply-timeout-ms": [ms] // optional; default is 10000
 *      },
 *      "rk" : "[routing key]",
 *      "lockout-key" : "[uuid]",  // optional
 *      "save" : "[filename]"  // optional
 *      "load" : "[filename]"  // optional; only used for cmd
 *  }
 */

#ifndef DRIPLINE_AGENT_HH_
#define DRIPLINE_AGENT_HH_

#include "core.hh"

#include "message.hh"

#include "param.hh"


namespace dripline
{
    class DRIPLINE_API agent : public core
    {
        public:
            agent( const scarab::param_node& a_node );
            ~agent();

            void execute();
            //void cancel();

            int get_return();
            mv_accessible_noset( reply_ptr_t, reply );

        private:
            request_ptr_t create_run_request( const std::string& a_routing_key );
            request_ptr_t create_get_request( const std::string& a_routing_key );
            request_ptr_t create_set_request( const std::string& a_routing_key );
            request_ptr_t create_cmd_request( const std::string& a_routing_key );

            scarab::param_node f_config;
            int f_return;
    };

} /* namespace dripline */

#endif /* DRIPLINE_AGENT_HH_ */
