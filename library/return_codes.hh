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
 *  New return codes can be defined in client code by including this header, and using the 3-line sequence above.
 *  Note that this must be done in the `dripline` namespace.  Resulting return codes will be in that namespace.
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

#include "dripline_error.hh"

#include "indexed_factory.hh"
#include "macros.hh"

#include <set>
#include <string>

namespace dripline
{
    // Base class
    struct DRIPLINE_API return_code
    {
        virtual ~return_code() {};
        virtual unsigned rc_value() const = 0;
        virtual std::string rc_name() const = 0;
    };


    // Macros for defining and implementing new return codes

#define DEFINE_DL_RET_CODE( name, api_macro ) \
    struct api_macro dl_##name : public ::dripline::return_code \
    { \
        static unsigned s_value; \
        static std::string s_name; \
        virtual ~dl_##name() {} \
        virtual unsigned rc_value() const { return dl_##name::s_value; } \
        virtual std::string rc_name() const {return dl_##name::s_name; } \
    };

#define DEFINE_DL_RET_CODE_NOAPI( name ) \
    struct dl_##name : public ::dripline::return_code \
    { \
        static unsigned s_value; \
        static std::string s_name; \
        virtual ~dl_##name() {} \
        virtual unsigned rc_value() const { return dl_##name::s_value; } \
        virtual std::string rc_name() const {return dl_##name::s_name; } \
    };

#define IMPLEMENT_DL_RET_CODE( name, the_value ) \
    unsigned dl_##name::s_value = the_value; \
    std::string dl_##name::s_name( TOSTRING(name) ); \
    static scarab::indexed_registrar< unsigned, ::dripline::return_code, dl_##name > t_dl_##name##_rc_reg( the_value );

//    std::string ::dripline::dl_##name::s_name = TOSTRING(name);

    //****************
    // Return code definitions
    //****************

    DEFINE_DL_RET_CODE( success, DRIPLINE_API );

    DEFINE_DL_RET_CODE( warning_no_action_taken, DRIPLINE_API );

    DEFINE_DL_RET_CODE( amqp_error, DRIPLINE_API );
    DEFINE_DL_RET_CODE( amqp_error_broker_connection, DRIPLINE_API );
    DEFINE_DL_RET_CODE( amqp_error_routingkey_notfound, DRIPLINE_API );

    DEFINE_DL_RET_CODE( device_error, DRIPLINE_API );
    DEFINE_DL_RET_CODE( device_error_connection, DRIPLINE_API );
    DEFINE_DL_RET_CODE( device_error_no_resp, DRIPLINE_API );

    DEFINE_DL_RET_CODE( message_error, DRIPLINE_API );
    DEFINE_DL_RET_CODE( message_error_no_encoding, DRIPLINE_API );
    DEFINE_DL_RET_CODE( message_error_decoding_fail, DRIPLINE_API );
    DEFINE_DL_RET_CODE( message_error_bad_payload, DRIPLINE_API );
    DEFINE_DL_RET_CODE( message_error_invalid_value, DRIPLINE_API );
    DEFINE_DL_RET_CODE( message_error_timeout, DRIPLINE_API );
    DEFINE_DL_RET_CODE( message_error_invalid_method, DRIPLINE_API );
    DEFINE_DL_RET_CODE( message_error_access_denied, DRIPLINE_API );
    DEFINE_DL_RET_CODE( message_error_invalid_key, DRIPLINE_API );
    DEFINE_DL_RET_CODE( message_error_dripline_deprecated, DRIPLINE_API );
    DEFINE_DL_RET_CODE( message_error_invalid_specifier, DRIPLINE_API );

    DEFINE_DL_RET_CODE( client_error, DRIPLINE_API );
    DEFINE_DL_RET_CODE( client_error_invalid_request, DRIPLINE_API );
    DEFINE_DL_RET_CODE( client_error_handling_reply, DRIPLINE_API );
    DEFINE_DL_RET_CODE( client_error_unable_to_send, DRIPLINE_API );
    DEFINE_DL_RET_CODE( client_error_timeout, DRIPLINE_API );

    DEFINE_DL_RET_CODE( unhandled_exception, DRIPLINE_API );

} /* namespace dripline */

#endif /* DRIPLINE_RETURN_CODES_HH_ */
