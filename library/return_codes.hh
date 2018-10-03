/*
 * return_codes.hh
 *
 *  Created on: Aug 25, 2018
 *      Author: N.S. Oblath
 *
 *  How to create a new return code:
 *
 *  In your header file:
 *
 *      DEFINE_DL_RET_CODE( [name] );
 *
 *  In your implementation file:
 *
 *      IMPL_DL_RET_CODE( [name], [value] );
 *
 */

#ifndef DRIPLINE_RETURN_CODES_HH_
#define DRIPLINE_RETURN_CODES_HH_

#include "dripline_api.hh"

#include "unique_typelist.hh"

namespace dripline
{

    // test implementation

    using rc_list_void = scarab::type_list<>;
#define RC_LIST rc_list_void

    typedef std::integral_constant< unsigned, 0 > success_t;
    using rc_list_success_t = scarab::append< success_t, RC_LIST >;
#undef RC_LIST
#define RC_LIST rc_list_success_t

    typedef std::integral_constant< unsigned, 1 > warning_t;
    using rc_list_warning_t = scarab::append< warning_t, RC_LIST >;
#undef RC_LIST
#define RC_LIST rc_list_warning_t

/*    // this should fail
    typedef std::integral_constant< unsigned, 1 > duplicate_t;
    using rc_list_duplicate_t = scarab::append< duplicate_t, RC_LIST >;
#undef RC_LIST
#define RC_LIST rc_list_duplicate_t
    //debug_t< RC_LIST > d;
*/




    struct DRIPLINE_API return_code
    {
        virtual ~return_code() {};
        virtual unsigned retcode() const = 0;
    };


#define DEFINE_DL_RET_CODE( name, the_value ) \
    struct DRIPLINE_API dl_##name : public return_code, public std::integral_constant< unsigned, the_value > \
    { \
        virtual ~dl_##name() {} \
        virtual unsigned retcode() const { return dl_##name::value; } \
    };
    
#define IMPL_DL_RET_CODE( name, the_code ) \
    const unsigned dl_##name::f_code = the_code;
    

    DEFINE_DL_RET_CODE( success, 0 );

    DEFINE_DL_RET_CODE( warning_no_action_taken, 1 );

    DEFINE_DL_RET_CODE( amqp_error, 100 );
    DEFINE_DL_RET_CODE( amqp_error_broker_connection, 101 );
    DEFINE_DL_RET_CODE( amqp_error_routingkey_notfound, 102 );

    DEFINE_DL_RET_CODE( device_error, 200 );
    DEFINE_DL_RET_CODE( device_error_connection, 201 );
    DEFINE_DL_RET_CODE( device_error_no_resp, 202 );

    DEFINE_DL_RET_CODE( message_error, 300 );
    DEFINE_DL_RET_CODE( message_error_no_encoding, 301 );
    DEFINE_DL_RET_CODE( message_error_decoding_fail, 302 );
    DEFINE_DL_RET_CODE( message_error_bad_payload, 303 );
    DEFINE_DL_RET_CODE( message_error_invalid_value, 304 );
    DEFINE_DL_RET_CODE( message_error_timeout, 305 );
    DEFINE_DL_RET_CODE( message_error_invalid_method, 306 );
    DEFINE_DL_RET_CODE( message_error_access_denied, 307 );
    DEFINE_DL_RET_CODE( message_error_invalid_key, 308 );
    DEFINE_DL_RET_CODE( message_error_dripline_deprecated, 309 );

    DEFINE_DL_RET_CODE( database_error, 400 );

    DEFINE_DL_RET_CODE( daq_error, 500 );
    DEFINE_DL_RET_CODE( daq_not_enabled, 501 );
    DEFINE_DL_RET_CODE( daq_running, 502 );

    DEFINE_DL_RET_CODE( unhandled_exception, 999 );

} /* namespace dripline */

#endif /* DRIPLINE_RETURN_CODES_HH_ */
