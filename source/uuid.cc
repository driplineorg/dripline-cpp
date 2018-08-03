/*
Copyright 2015 Noah S. Oblath

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
 * uuid.cc
 *
 *  Created on: Sep 16, 2015
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "uuid.hh"

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>


namespace dripline
{
    uuid_t DRIPLINE_API generate_random_uuid()
    {
        static boost::uuids::random_generator t_gen;

        return t_gen();
    }

    uuid_t DRIPLINE_API generate_nil_uuid()
    {
        return boost::uuids::nil_uuid();
    }

    uuid_t DRIPLINE_API uuid_from_string( const std::string& a_id_str )
    {
        static boost::uuids::string_generator t_gen;

        if( a_id_str.empty() ) return generate_nil_uuid();

        try
        {
            return t_gen( a_id_str );
        }
        catch(...)
        {
            throw;
        }
    }

    uuid_t DRIPLINE_API uuid_from_string( const char* a_id_str )
    {
        static boost::uuids::string_generator t_gen;

        if( strcmp( a_id_str, "" ) == 0 ) return generate_nil_uuid();

        try
        {
            return t_gen( a_id_str );
        }
        catch(...)
        {
            throw;
        }
    }

    uuid_t DRIPLINE_API uuid_from_string( const std::string& a_id_str, bool& a_valid_flag )
    {
        a_valid_flag = true;
        try
        {
            return uuid_from_string( a_id_str );
        }
        catch(...)
        {
            a_valid_flag = false;
            return generate_nil_uuid();
        }
    }

    uuid_t DRIPLINE_API uuid_from_string( const char* a_id_str, bool& a_valid_flag )
    {
        a_valid_flag = true;
        try
        {
            return uuid_from_string( a_id_str );
        }
        catch(...)
        {
            a_valid_flag = false;
            return generate_nil_uuid();
        }
    }


    std::string DRIPLINE_API string_from_uuid( const uuid_t& a_id )
    {
        return boost::uuids::to_string( a_id );
    }


} /* namespace dripline */
