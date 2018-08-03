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
 * routing_key_specifier.hh
 *
 *  Created on: Feb 18, 2016
 *      Author: nsoblath
 */

#ifndef DRIPLINE_ROUTING_KEY_SPECIFIER_HH_
#define DRIPLINE_ROUTING_KEY_SPECIFIER_HH_

#include "dripline_api.hh"

#include <deque>
#include <string>

namespace dripline
{

    class DRIPLINE_API routing_key_specifier : public std::deque< std::string >
    {
        public:
            typedef std::deque< std::string > container_type;

        public:
            routing_key_specifier( const std::string& a_rk = "" );
            virtual ~routing_key_specifier();

            void parse( const std::string& a_rk );
            std::string to_string() const;

        private:
            void add_next( const std::string& a_addr );

        public:
            static const char f_node_separator = '.';

    };

} /* namespace dripline */

#endif /* DRIPLINE_ROUTING_KEY_SPECIFIER_HH_ */
