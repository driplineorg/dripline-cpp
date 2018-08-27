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

#define DEFINE_DL_RET_CODE( name ) \
    class DRIPLINE_API dl_##name : public return_code \
    { \
        public: \
            dl_##name() : return_code() {} \
            virtual ~dl_##name() {} \
            static const unsigned f_code; \
    };
    
#define IMPL_DL_RET_CODE( name, the_code ) \
    const unsigned dl_##name::f_code = the_code;
    
    DEFINE_DL_RET_CODE( success );

    DEFINE_DL_RET_CODE( warning_no_action_taken );

    DEFINE_DL_RET_CODE( amqp_error );
    DEFINE_DL_RET_CODE( amqp_error_broker_connection );
    DEFINE_DL_RET_CODE( amqp_error_routingkey_notfound );

    DEFINE_DL_RET_CODE( device_error );
    DEFINE_DL_RET_CODE( device_error_connection );
    DEFINE_DL_RET_CODE( device_error_no_resp );

    DEFINE_DL_RET_CODE( message_error );
    DEFINE_DL_RET_CODE( message_error_no_encoding );
    DEFINE_DL_RET_CODE( message_error_decoding_fail );
    DEFINE_DL_RET_CODE( message_error_bad_payload );
    DEFINE_DL_RET_CODE( message_error_invalid_value );
    DEFINE_DL_RET_CODE( message_error_timeout );
    DEFINE_DL_RET_CODE( message_error_invalid_method );
    DEFINE_DL_RET_CODE( message_error_access_denied );
    DEFINE_DL_RET_CODE( message_error_invalid_key );
    DEFINE_DL_RET_CODE( message_error_dripline_deprecated );

    DEFINE_DL_RET_CODE( database_error );

    DEFINE_DL_RET_CODE( daq_error );
    DEFINE_DL_RET_CODE( daq_not_enabled );
    DEFINE_DL_RET_CODE( daq_running );

    DEFINE_DL_RET_CODE( unhandled_exception );


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
