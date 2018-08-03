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

#ifndef DRIPLINE_ERROR_HH_
#define DRIPLINE_ERROR_HH_

#include <sstream>
#include <exception>

#include "dripline_constants.hh"

#include "dripline_api.hh"

namespace dripline
{

    class DRIPLINE_API dripline_error : public ::std::exception
    {
        public:
            dripline_error();
            dripline_error( const dripline_error& );
            ~dripline_error() throw ();

            template< class x_streamable >
            dripline_error& operator<<( const x_streamable& a_fragment )
            {
                f_error << a_fragment;
                return *this;
            }

            dripline_error& operator<<( retcode_t a_code )
            {
                f_retcode = a_code;
                return *this;
            }

            virtual const char* what() const throw();

            retcode_t retcode() const
            {
                return f_retcode;
            }

        private:
            ::std::stringstream f_error;
            retcode_t f_retcode;
    };

}

#endif /* DRIPLINE_ERROR_HH_ */
