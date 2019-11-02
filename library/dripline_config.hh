/*
 * dripline_config.hh
 *
 *  Created on: June 26, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_DRIPLINE_CONFIG_HH_
#define DRIPLINE_DRIPLINE_CONFIG_HH_

#include "dripline_api.hh"

#include "param.hh"

namespace scarab
{
    class main_app;
}

namespace dripline
{

    /*!
     @class dripline_config
     @author N.S. Oblath

     @brief Sets the default configuration used by `core`.
    */
    class DRIPLINE_API dripline_config : public scarab::param_node
    {
        public:
            dripline_config();
            virtual ~dripline_config();
    };

    /// Add basic AMQP options to an app object
    void add_dripline_options( scarab::main_app& an_app );

} /* namespace dripline */
#endif /* DRIPLINE_DRIPLINE_CONFIG_HH_ */
