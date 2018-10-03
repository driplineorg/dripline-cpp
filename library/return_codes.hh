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

#include <type_traits>

namespace dripline
{
    // unique type check
    template< typename T, typename FirstType, typename... OtherTypes  > struct mp_is_unique_impl;

    template< typename T, typename FirstType > struct mp_is_unique_impl< T, FirstType >
    {
        static const bool value = ! std::is_same< T, FirstType >::value;
    };

    template< typename T, typename FirstType, typename... OtherTypes  > struct mp_is_unique_impl
    {
        static const bool value = std::is_same< T, FirstType >::value ? false : mp_is_unique_impl< T, OtherTypes... >::value;
    };

    template< typename T, typename FirstType, typename... OtherTypes >
    struct mp_is_unique : std::integral_constant< bool, mp_is_unique_impl< T, FirstType, OtherTypes... >::value >
    {};

    // type list
    template< typename... Types > struct mp_list {};

    // appending
    template< typename T, typename... ListItems > struct mp_append_impl;

    template< typename T >
    struct mp_append_impl< T, mp_list<> >
    {
        using type = mp_list< T >;
    };

    template< typename T, typename... ListItems >
    struct mp_append_impl< T, mp_list< ListItems... > >
    {
        static_assert(mp_is_unique< T, ListItems... >::value, "Non-unique return code found");
        using type = mp_list< T, ListItems... >;
    };

    template< typename T, typename List >
    using mp_append = typename mp_append_impl< T, List >::type;

    // for debug printing
    template< typename T > struct debug_t;


    // test implementation

    using rc_list_void = mp_list<>;
#define RC_LIST rc_list_void

    typedef std::integral_constant< unsigned, 0 > success_t;
    using rc_list_success_t = mp_append< success_t, RC_LIST >;
#undef RC_LIST
#define RC_LIST rc_list_success_t

    typedef std::integral_constant< unsigned, 1 > warning_t;
    using rc_list_warning_t = mp_append< warning_t, RC_LIST >;
#undef RC_LIST
#define RC_LIST rc_list_warning_t

/*    // this should fail
    typedef std::integral_constant< unsigned, 1 > duplicate_t;
    using rc_list_duplicate_t = mp_append< duplicate_t, RC_LIST >;
#undef RC_LIST
#define RC_LIST rc_list_duplicate_t
    //debug_t< RC_LIST > d;
*/


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
