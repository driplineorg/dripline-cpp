/*
 * service_config.hh
 *
 *  Created on: Aug 14, 2024
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_SERVICE_CONFIG_HH_
#define DRIPLINE_SERVICE_CONFIG_HH_

#include "dripline_config.hh" // for convenience of things using service_config, include this here

namespace dripline
{

    /*!
     @class service_config
     @author N.S. Oblath

     @brief Sets the default service configuration
    */
    class DRIPLINE_API service_config : public scarab::param_node
    {
        public:
            /// Constructor: name is available as a parameter since it needs to be unique for each service
            service_config( const std::string& a_name = "a_service" );
            virtual ~service_config();
    };

} /* namespace dripline */
#endif /* DRIPLINE_SERVICE_CONFIG_HH_ */
