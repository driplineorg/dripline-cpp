/*
 * reply_package.hh
 *
 *  Created on: Apr 26, 2018
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_REPLY_PACKAGE_HH_
#define DRIPLINE_REPLY_PACKAGE_HH_

#include "dripline_api.hh"
#include "dripline_error.hh"
#include "service.hh"

#include <memory>

namespace dripline
{
    struct DRIPLINE_API reply_info
    {
        bool f_send_result;
        retcode_t f_return_code;
        std::string f_return_msg;
        reply_info( bool a_send_result, retcode_t a_return_code, const std::string& a_return_msg );
        bool operator bool() const;
    };

    struct DRIPLINE_API reply_package
    {
        const service* f_service_ptr;
        std::string f_reply_to;
        std::string f_correlation_id;
        scarab::param_node f_payload;
        reply_package( const service* a_service, request_ptr_t a_request );
        reply_info send_reply( retcode_t a_return_code, const std::string& a_return_msg ) const;
        reply_info send_reply( const dripline_error& an_error ) const;
    };

    inline bool reply_info::operator bool() const
    {
        return f_send_result;
    }

    inline reply_info reply_package::send_reply( const dripline_error& an_error ) const
    {
        return send_reply( an_error.retcode(), an_error.what() );
    }
}

#endif /* DRIPLINE_REPLY_PACKAGE_HH_ */
