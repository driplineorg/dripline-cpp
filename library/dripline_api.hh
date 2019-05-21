/*
 * mantis_api.hh
 *
 *  Created on: Jan 1, 2016
 *      Author: nsoblath
 *
 *  This file contains the macros that allow Dripline to be compiled into a Windows DLL.
 *  This does not define the actual API for the Dripline library, or anything like that.
 */

#ifndef DRIPLINE_API_HH_
#define DRIPLINE_API_HH_

#include "scarab_api.hh"

namespace dripline
{
    // API export macros for windows
#ifdef _WIN32
#  ifdef DRIPLINE_API_EXPORTS
#    define DRIPLINE_API __declspec(dllexport)
#    define DRIPLINE_EXPIMP_TEMPLATE
#  else
#    define DRIPLINE_API __declspec(dllimport)
#    define DRIPLINE_EXPIMP_TEMPLATE extern
#  endif

#  ifdef DRIPLINE_EXAMPLES_API_EXPORTS
#    define DRIPLINE_EXAMPLES_API __declspec(dllexport)
#  else
#    define DRIPLINE_EXAMPLES_API __declspec(dllimport)
#  endif
#else
#  define DRIPLINE_API
#  define DRIPLINE_EXAMPLES_API
#endif

}

#endif /* DRIPLINE_API_HH_ */
