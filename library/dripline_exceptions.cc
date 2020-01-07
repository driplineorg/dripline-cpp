/*
 * dripline_exceptions.cc
 *
 *  Created on: Aug 14, 2018
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "dripline_exceptions.hh"

namespace dripline
{

    dripline_error::dripline_error() :
            base_exception< dripline_error >()
    {}

    dripline_error::dripline_error( const dripline_error& an_error ) :
            base_exception< dripline_error >( an_error )
    {}

    dripline_error::~dripline_error() noexcept
    {}

}
