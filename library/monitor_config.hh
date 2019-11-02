/*
 * monitor_config.hh
 *
 *  Created on: Jul 3, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_MONITOR_CONFIG_HH_
#define DRIPLINE_MONITOR_CONFIG_HH_

#include "dripline_config.hh" // for convenience of things using monitor_config, include this here

namespace dripline
{

    /*!
     @class monitor_config
     @author N.S. Oblath

     @brief Sets the default message-monitor configuration
    */
    class DRIPLINE_API monitor_config : public scarab::param_node
    {
        public:
            monitor_config();
            virtual ~monitor_config();
    };

} /* namespace dripline */
#endif /* DRIPLINE_MONITOR_CONFIG_HH_ */
