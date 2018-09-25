/*
 * return_codes.hh
 *
 *  Created on: Aug 25, 2018
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_RETURN_CODES_HH_
#define DRIPLINE_RETURN_CODES_HH_

#include "dripline_api.hh"

namespace dripline
{

    struct DRIPLINE_API return_code
    {
        virtual ~return_code() {};
        virtual unsigned retcode() const = 0;
    };


#define DEFINE_DL_RET_CODE( name ) \
    struct DRIPLINE_API dl_##name : public return_code \
    { \
        virtual ~dl_##name() {} \
        virtual unsigned retcode() const { return dl_##name::f_code; } \
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

} /* namespace dripline */

#endif /* DRIPLINE_RETURN_CODES_HH_ */
