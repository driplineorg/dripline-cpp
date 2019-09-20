/*
 * dripline_version.hh
 *
 *  Created on: Mar 20, 2013
 *      Author: nsoblath
 */

#ifndef DRIPLINE_VERSION_HH_
#define DRIPLINE_VERSION_HH_

#include "singleton.hh"

#include "dripline_api.hh"

#include "scarab_version.hh"

#include <string>


namespace dripline
{

    class DRIPLINE_API version : public scarab::version_semantic
    {
        public:
            version();
            ~version();
    };

    class DRIPLINE_API version_dripline_protocol : public scarab::version_semantic
    {
        public:
            version_dripline_protocol();
            ~version_dripline_protocol();
    };

} // namespace dripline

#endif /* DRIPLINE_VERSION_HH_ */
