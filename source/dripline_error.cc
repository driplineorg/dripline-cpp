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


#define DRIPLINE_API_EXPORTS

#include "dripline_error.hh"

namespace dripline
{

    dripline_error::dripline_error() :
            ::std::exception(),
            f_error( "" ),
            f_retcode( retcode_t::success )
    {
    }

    dripline_error::dripline_error( const dripline_error& an_error ) :
            std::exception(),
            f_error( an_error.f_error.str() ),
            f_retcode( an_error.f_retcode )
    {
    }

    dripline_error::~dripline_error() throw ()
    {
    }

    const char* dripline_error::what() const throw ()
    {
        return f_error.str().c_str();
    }

}
