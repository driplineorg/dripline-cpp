/*
 * agent_config.hh
 *
 *  Created on: Jun 2, 2016
 *      Author: nsoblath
 */

#ifndef DRIPLINE_AGENT_CONFIG_HH_
#define DRIPLINE_AGENT_CONFIG_HH_

#include "dripline_config.hh" // for convenience of things using agent_config, include this here

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
