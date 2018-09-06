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
#include "param.hh"

namespace dripline
{

    struct DRIPLINE_API return_code
    {
        virtual ~return_code() {};
        virtual unsigned retcode() const = 0;
    };

    // TODO: move this to reply_package.hh/cc and rename
    class DRIPLINE_API reply_package_2
    {
        public:
            reply_package_2();
            template< typename x_retcode >
            reply_package_2( const std::string& a_message, scarab::param_ptr_t a_payload = nullptr );
            reply_package_2( const reply_package_2& a_orig ) = delete;
            reply_package_2( reply_package_2&& a_orig );
            virtual ~reply_package_2();

            reply_package_2& operator=( const reply_package_2& a_orig ) = delete;
            reply_package_2& operator=( reply_package_2&& a_orig );

        public:
            unsigned retcode() const;

            template< typename x_retcode >
            reply_package_2& set_retcode();

            scarab::param& payload();
            const scarab::param& payload() const;

            reply_package_2& set_payload( scarab::param_ptr_t a_payload );

            template< class x_streamable >
            reply_package_2& operator<<( x_streamable a_fragment );
            reply_package_2& operator<<( const std::string& a_fragment );
            reply_package_2& operator<<( const char* a_fragment );

            mv_referrable( std::string, message );

        protected:
            std::unique_ptr< return_code > f_code;

            scarab::param_ptr_t f_payload;
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


    template< typename x_retcode >
    reply_package_2::reply_package_2( const std::string& a_message, scarab::param_ptr_t a_payload = nullptr ) :
        f_message( a_message ),
        f_code( new x_retcode() ),
        f_payload( a_payload )
    {

    }

    template< class x_streamable >
    reply_package_2& reply_package_2::operator<<( x_streamable a_fragment )
    {
        std::stringstream stream;
        stream << a_fragment;
        stream >> f_message;
        return *this;
    }

    inline reply_package_2& reply_package_2::operator<<( const std::string& a_fragment )
    {
        f_message += a_fragment;
        return *this;
    }

    inline reply_package_2& reply_package_2::operator<<( const char* a_fragment )
    {
        f_message += std::string( a_fragment );
        return *this;
    }

    inline scarab::param& reply_package_2::payload()
    {
        return *f_payload;
    }

    inline const scarab::param& reply_package_2::payload() const
    {
        return *f_payload;
    }

    inline unsigned reply_package_2::retcode() const
    {
        return f_code->retcode();
    }

    template< typename x_retcode >
    reply_package_2& reply_package_2::set_retcode()
    {
        f_code.reset( new x_retcode() );
        return *this;
    }

    reply_package_2& reply_package_2::set_payload( scarab::param_ptr_t a_payload )
    {
        f_payload = std::move(a_payload);
        return *this;
    }

} /* namespace dripline */

#endif /* DRIPLINE_RETURN_CODES_HH_ */
