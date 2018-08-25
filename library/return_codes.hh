/*
 * return_codes.hh
 *
 *  Created on: Aug 25, 2018
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_RETURN_CODES_HH_
#define DRIPLINE_RETURN_CODES_HH_

#include <sstream>
#include <string>

#include "dripline_api.hh"

#include "member_variables.hh"

namespace dripline
{

    class DRIPLINE_API return_code
    {
        public:
            return_code();
            return_code( const return_code& a_orig );
            return_code( return_code&& a_orig );
            virtual ~return_code();

            return_code& operator=( const return_code& a_orig );
            return_code& operator=( return_code&& a_orig );

            template< class x_streamable >
            return_code& operator<<( x_streamable a_fragment );
            return_code& operator<<( const std::string& a_fragment );
            return_code& operator<<( const char* a_fragment );

            mv_referrable( std::string, message );
    };

    class DRIPLINE_API return_success : public return_code
    {
        public:
            return_success() : return_code() {}
            virtual ~return_success() {}

            static unsigned f_code;
    };

    template< class x_streamable >
    return_code& return_code::operator<<( x_streamable a_fragment )
    {
        std::stringstream stream;
        stream << a_fragment;
        stream >> f_message;
        return *this;
    }

    inline return_code& return_code::operator<<( const std::string& a_fragment )
    {
        f_message += a_fragment;
        return *this;
    }

    inline return_code& return_code::operator<<( const char* a_fragment )
    {
        f_message += std::string( a_fragment );
        return *this;
    }


} /* namespace dripline */

#endif /* DRIPLINE_RETURN_CODES_HH_ */
