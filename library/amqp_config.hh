/*
 * amqp_config.hh
 *
 *  Created on: June 26, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_AMQP_CONFIG_HH_
#define DRIPLINE_AMQP_CONFIG_HH_

#include "dripline_api.hh"

#include "param.hh"

namespace scarab
{
    class main_app;
}

namespace dripline
{

    class DRIPLINE_API amqp_config : public scarab::param_node
    {
        public:
            amqp_config();
            virtual ~amqp_config();
    };

    /// Add basic AMQP options to an app object
    void add_amqp_options( scarab::main_app& an_app );

} /* namespace dripline */
#endif /* DRIPLINE_AMQP_CONFIG_HH_ */
