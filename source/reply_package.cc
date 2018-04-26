/*
 * reply_package.cc
 *
 *  Created on: Apr 26, 2018
 *      Author: obla999
 */

#include "reply_package.hh"

#include "logger.hh"


namespace dripline
{
    LOGGER( dlog, "reply_package" );

    reply_info::reply_info( bool a_send_result, retcode_t a_return_code, const std::string& a_return_msg ) :
            f_send_result( a_send_result ),
            f_return_code( a_return_code ),
            f_return_msg( a_return_msg )
    {}

    reply_package::reply_package( const service* a_service, request_ptr_t a_request ) :
        f_service_ptr( a_service ),
        f_reply_to( a_request->reply_to() ),
        f_correlation_id( a_request->correlation_id() ),
        f_payload()
    {}

    reply_info reply_package::send_reply( retcode_t a_return_code, const std::string& a_return_msg ) const
    {
        reply_info t_reply_info( false, a_return_code, a_return_msg );

        if( f_service_ptr == nullptr )
        {
            LWARN( dlog, "Service pointer is null; Unable to send reply" );
            return t_reply_info;
        }

        reply_ptr_t t_reply = msg_reply::create( a_return_code, a_return_msg, new scarab::param_node( f_payload ), f_reply_to, message::encoding::json );
        t_reply->correlation_id() = f_correlation_id;

        LDEBUG( dlog, "Sending reply message to <" << f_reply_to << ">:\n" <<
                 "Return code: " << t_reply->get_return_code() << '\n' <<
                 "Return message: " << t_reply->return_msg() <<
                 f_payload );

        if ( f_reply_to.empty() )
        {
            //TODO should this be PROG?
            LPROG( dlog, "Not sending reply (reply-to empty)\n" <<
                         "    Return code: " << t_reply->get_return_code() << '\n' <<
                         "    Return message: " << t_reply->return_msg() << f_payload );
        }
        else if( ! f_service_ptr->core::send( t_reply ) )
        {
            LWARN( dlog, "Something went wrong while sending the reply" );
            return t_reply_info;
        }

        t_reply_info.f_return_code = true;
        return t_reply_info;
    }

}
