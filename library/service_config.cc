/*
 * service_config.cc
 *
 *  Created on: Aug 24, 2024
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "service_config.hh"

//#include "logger.hh"

//LOGGER( dlog, "service_config" );

namespace dripline
{

    service_config::service_config( const std::string& a_name )
    {
        // default service configuration

        add( "dripline", dripline_config() );

        add( "name", a_name ); // seconds
    }

    //service_config::~service_config()
    //{
    //}

} /* namespace dripline */
