/*
 * @file return_codes.hh
 *
 *  Created on: Aug 25, 2018
 *      Author: N.S. Oblath
 *
 *  This file contains the basis for the extensible return code system.
 *  Return codes have a name and a value (unsigned integer).
 *  The class name corresponding to your return code will be dl_[name].
 *  The integer values must be unique, and that uniqueness is enforced in dripline-cpp at run time.
 *
 *  New return codes are created using the macro DEFINE_DL_RET_CODE or DEFINE_DL_RET_CODE_NOAPI in a header file,
 *  and the macro IMPLEMENT_DL_RET_CODE in a source file.
 * 
 *  This file includes the definitions for all Dripline-standard return codes.  They are implemented in return_codes.cc.
 */

#ifndef DRIPLINE_RETURN_CODES_HH_
#define DRIPLINE_RETURN_CODES_HH_

#include "dripline_api.hh"

#include "indexed_factory.hh"
#include "macros.hh"

#include <set>
#include <string>

namespace dripline
{
    /*!
     @class return_code
     @author N.S. Oblath
     @brief Base class for return codes
    */
     struct DRIPLINE_API return_code
    {
        virtual ~return_code() {};
        virtual unsigned rc_value() const = 0;
        virtual std::string rc_name() const = 0;
        virtual std::string rc_description() const = 0;
    };

    /*!
     @class copy_code
     @author N.S. Oblath
     @brief Stores a copy of the return-code value, name, and description from any return_code-derived class
    */
     struct DRIPLINE_API copy_code : return_code
    {
        copy_code( const return_code& a_code );
        virtual ~copy_code() {};
        virtual unsigned rc_value() const { return f_value; }
        virtual std::string rc_name() const { return f_name; }
        virtual std::string rc_description() const { return f_description; }
        unsigned f_value;
        std::string f_name;
        std::string f_description;
    };

    // Macros for defining and implementing new return codes

    /*!
     \def DEFINE_DL_RET_CODE( name, api_macro )
     Defines a return_code object with class name `dl_[name]`, using an API macro (e.g. for going in a Window DLL).
     This macro should go in a header file.
    */
#define DEFINE_DL_RET_CODE( name, api_macro ) \
    struct api_macro dl_##name : public ::dripline::return_code \
    { \
        static unsigned s_value; \
        static std::string s_name; \
        static std::string s_description; \
        virtual ~dl_##name() {} \
        virtual unsigned rc_value() const { return dl_##name::s_value; } \
        virtual std::string rc_name() const {return dl_##name::s_name; } \
        virtual std::string rc_description() const {return dl_##name::s_description; } \
    };

    /*!
     \def DEFINE_DL_RET_CODE_NOAPI( name )
     Defines a return_code object with class name `dl_[name]`, with no API macro.
     This macro should go in a header file.
    */
#define DEFINE_DL_RET_CODE_NOAPI( name ) \
    struct dl_##name : public ::dripline::return_code \
    { \
        static unsigned s_value; \
        static std::string s_name; \
        static std::string s_description; \
        virtual ~dl_##name() {} \
        virtual unsigned rc_value() const { return dl_##name::s_value; } \
        virtual std::string rc_name() const {return dl_##name::s_name; } \
        virtual std::string rc_description() const {return dl_##name::s_description; } \
    };

    /*!
     \def IMPLEMENT_DL_RET_CODE( name, the_value )
     Implement the return_code object with class name `dl_[name]`, and give it the value `the_value`.
     `the_value` should be an unsigned integer.
     This macro should go in a source file.
    */
#define IMPLEMENT_DL_RET_CODE( name, the_value, description ) \
    unsigned dl_##name::s_value = the_value; \
    std::string dl_##name::s_name( TOSTRING(name) ); \
    std::string dl_##name::s_description( description );\
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
