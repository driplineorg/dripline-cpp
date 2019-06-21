/*
 * return_codes.hh
 *
 *  Created on: Aug 25, 2018
 *      Author: N.S. Oblath
 *
 *  This file contains the basis for the extensible return code system.
 *  Return codes have a name and a value (unsigned integer).
 *  The class name corresponding to your return code will be dl_[name].
 *  The integer values must be unique, and that uniqueness is enforced in dripline-cpp at compile time.
 *
 *  New return codes are created using a macro, DEFINE_DL_RET_CODE, plus a two other simple lines.
 *  Place the follwoing in your header file for each return code you want to add:
 *
 *      DEFINE_DL_RET_CODE( [name], [value] );
 *  #undef RC_LIST
 *  #define RC_LIST NEW_RC_LIST( [name] )
 *
 *  Technical details:
 *
 *  - All return code classes inherit from the class return_code and std::integral_constant< unsigned, [value] >.
 *  - Uniqueness of the values is enforced by a type list on which a compile-time algorithm is run to ensure the values are unique
 *    when they're added to the list (see scarab/library/utility/unique_typelist.hh).
 *  - The redefinitions of RC_LIST are necessary to use the append function and continuously add types to the list.
 */

#ifndef DRIPLINE_RETURN_CODES_HH_
#define DRIPLINE_RETURN_CODES_HH_

#include "dripline_api.hh"

#include "unique_typelist.hh"

namespace dripline
{
    // Base class
    struct DRIPLINE_API return_code
    {
        virtual ~return_code() {};
        virtual unsigned rc_value() const = 0;
    };

    // Macros for defining new return codes
#define NEW_RC_LIST( name ) rc_list_##name##_t

#define DEFINE_DL_RET_CODE( name, the_value ) \
    struct DRIPLINE_API dl_##name : public return_code, public std::integral_constant< unsigned, the_value > \
    { \
        virtual ~dl_##name() {} \
        virtual unsigned rc_value() const { return dl_##name::value; } \
    }; \
    using NEW_RC_LIST(name) = scarab::unique_append< std::integral_constant< unsigned, the_value >, RC_LIST >;

    // Start off the typelist as an empty list
    using rc_list_void = scarab::type_list<>;
#define RC_LIST rc_list_void


    //****************
    // Return codes
    //****************


    DEFINE_DL_RET_CODE( success, 0 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( success )

    DEFINE_DL_RET_CODE( warning_no_action_taken, 1 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( warning_no_action_taken )

    DEFINE_DL_RET_CODE( amqp_error, 100 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( amqp_error )
    DEFINE_DL_RET_CODE( amqp_error_broker_connection, 101 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( amqp_error_broker_connection )
    DEFINE_DL_RET_CODE( amqp_error_routingkey_notfound, 102 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( amqp_error_routingkey_notfound )

    DEFINE_DL_RET_CODE( device_error, 200 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( device_error )
    DEFINE_DL_RET_CODE( device_error_connection, 201 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( device_error_connection )
    DEFINE_DL_RET_CODE( device_error_no_resp, 202 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( device_error_no_resp )

    DEFINE_DL_RET_CODE( message_error, 300 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( message_error )
    DEFINE_DL_RET_CODE( message_error_no_encoding, 301 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( message_error_no_encoding )
    DEFINE_DL_RET_CODE( message_error_decoding_fail, 302 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( message_error_decoding_fail )
    DEFINE_DL_RET_CODE( message_error_bad_payload, 303 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( message_error_bad_payload )
    DEFINE_DL_RET_CODE( message_error_invalid_value, 304 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( message_error_invalid_value )
    DEFINE_DL_RET_CODE( message_error_timeout, 305 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( message_error_timeout )
    DEFINE_DL_RET_CODE( message_error_invalid_method, 306 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( message_error_invalid_method )
    DEFINE_DL_RET_CODE( message_error_access_denied, 307 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( message_error_access_denied )
    DEFINE_DL_RET_CODE( message_error_invalid_key, 308 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( message_error_invalid_key )
    DEFINE_DL_RET_CODE( message_error_dripline_deprecated, 309 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( message_error_dripline_deprecated )

    DEFINE_DL_RET_CODE( database_error, 400 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( database_error )

    DEFINE_DL_RET_CODE( client_error, 500 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( client_error )
    DEFINE_DL_RET_CODE( client_error_invalid_request, 501 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( client_error_invalid_request )
    DEFINE_DL_RET_CODE( client_error_handling_reply, 502 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( client_error_handling_reply )
    DEFINE_DL_RET_CODE( client_error_unable_to_send, 503 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( client_error_unable_to_send )
    DEFINE_DL_RET_CODE( client_error_timeout, 504 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( client_error_timeout )

    DEFINE_DL_RET_CODE( unhandled_exception, 999 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( unhandled_exception )

    // to test a duplicate error code:
    /*
    DEFINE_DL_RET_CODE( unhandled_exception_2, 999 );
#undef RC_LIST
#define RC_LIST NEW_RC_LIST( unhandled_exception_2 )
    */

} /* namespace dripline */

#endif /* DRIPLINE_RETURN_CODES_HH_ */
