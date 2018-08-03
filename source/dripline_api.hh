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
#else
#  define DRIPLINE_API
#endif
}

#endif /* DRIPLINE_API_HH_ */
