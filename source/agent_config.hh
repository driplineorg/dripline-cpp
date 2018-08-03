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
 * agent_config.hh
 *
 *  Created on: Jun 2, 2016
 *      Author: nsoblath
 */

#ifndef DRIPLINE_AGENT_CONFIG_HH_
#define DRIPLINE_AGENT_CONFIG_HH_

#include "dripline_api.hh"

#include "param.hh"

namespace dripline
{

    class DRIPLINE_API agent_config : public scarab::param_node
    {
        public:
            agent_config();
            virtual ~agent_config();
    };

} /* namespace dripline */
#endif /* DRIPLINE_AGENT_CONFIG_HH_ */
