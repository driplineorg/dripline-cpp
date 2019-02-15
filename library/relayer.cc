#define DRIPLINE_API_EXPORTS

#include "relayer.hh"

#include "dripline_error.hh"

#include "logger.hh"
#include "param.hh"

#include <chrono>

namespace dripline
{
    LOGGER( dlog, "relayer" );

    relayer::relayer( const scarab::param_node& a_config, const std::string& a_broker_address, unsigned a_port, const std::string& a_auth_file ) :
            core( a_config, a_broker_address, a_port, a_auth_file ),
            scarab::cancelable(),
            f_queue()
    {
    }

    relayer::~relayer()
    {
    }

    void relayer::execute_relayer()
    {
        LDEBUG( dlog, "Dripline relayer starting" );
        while( ! is_canceled() )
        {
            mar_ptr t_mar;
            bool t_have_message = f_queue.timed_wait_and_pop( t_mar ); // blocking call for next message to send; timed so that cancellation can be rechecked
            if( ! t_have_message ) continue;

            try
            {
                std::unique_lock< std::mutex > lock( t_mar->f_wait_for_send_pkg->f_mutex );
                switch( t_mar->f_message->message_type() )
                {
                    case msg_t::request:
                        t_mar->f_wait_for_send_pkg->f_sent_msg_pkg_ptr = core::send( std::static_pointer_cast< dripline::msg_request >( t_mar->f_message ) );
                        break;
                    case msg_t::alert:
                        t_mar->f_wait_for_send_pkg->f_sent_msg_pkg_ptr = core::send( std::static_pointer_cast< dripline::msg_alert >( t_mar->f_message ) );
                        break;
                    default:
                        throw dripline_error() << "Unsupported message type: " << t_mar->f_message->message_type();
                        break;
                }
                if( ! t_mar->f_wait_for_send_pkg->f_sent_msg_pkg_ptr->f_successful_send )
                {
                    LWARN( dlog, "Message sending failed: " << t_mar->f_wait_for_send_pkg->f_sent_msg_pkg_ptr->f_send_error_message << '\n' << *t_mar->f_message );
                }
                t_mar->f_wait_for_send_pkg->f_condition_var.notify_one();
                continue;
            }
            catch( dripline_error& e )
            {
                LWARN( dlog, "Unable to send message: " << e.what() << '\n' << *t_mar->f_message );
            }

        }

        LDEBUG( dlog, "Exiting the Dripline relayer" );

        return;
    }


    void relayer::do_cancellation()
    {
        LDEBUG( dlog, "Canceling relayer" );
        f_queue.interrupt();
        return;
    }

    relayer::wait_for_send_pkg_ptr relayer::send_async( request_ptr_t a_request ) const
    {
        if( is_canceled() )
        {
            LWARN( dlog, "Relayer has been canceled; request not sent" );
            wait_for_send_pkg_ptr t_return;
            t_return->f_sent_msg_pkg_ptr = std::make_shared< sent_msg_pkg >();
            t_return->f_sent_msg_pkg_ptr->f_successful_send = false;
            return t_return;
        }
        LDEBUG( dlog, "Sending request to <" << a_request->routing_key() << ">" );
        mar_ptr t_mar = std::make_shared< message_and_reply >();
        std::unique_lock< std::mutex > lock( t_mar->f_wait_for_send_pkg->f_mutex );
        t_mar->f_message = std::static_pointer_cast< dripline::message >( a_request );
        t_mar->f_wait_for_send_pkg = std::make_shared< wait_for_send_pkg >();
        f_queue.push( t_mar );
        return t_mar->f_wait_for_send_pkg;
    }

    relayer::wait_for_send_pkg_ptr relayer::send_async( alert_ptr_t a_alert ) const
    {
        if( is_canceled() )
        {
            LWARN( dlog, "Relayer has been canceled; request not sent" );
            wait_for_send_pkg_ptr t_return;
            t_return->f_sent_msg_pkg_ptr = std::make_shared< sent_msg_pkg >();
            t_return->f_sent_msg_pkg_ptr->f_successful_send = false;
            return t_return;
        }
        LDEBUG( dlog, "Sending alert to <" << a_alert->routing_key() << ">" );
        mar_ptr t_mar = std::make_shared< message_and_reply >();
        std::unique_lock< std::mutex > lock( t_mar->f_wait_for_send_pkg->f_mutex );
        t_mar->f_message = std::static_pointer_cast< dripline::message >( a_alert );
        t_mar->f_wait_for_send_pkg = std::make_shared< wait_for_send_pkg >();
        f_queue.push( t_mar );
        return t_mar->f_wait_for_send_pkg;
    }

    reply_ptr_t relayer::wait_for_reply( const wait_for_send_pkg_ptr a_receive_reply, int a_timeout_ms )
    {
        bool t_temp;
        return wait_for_reply( a_receive_reply, t_temp, a_timeout_ms );
    }

    reply_ptr_t relayer::wait_for_reply( const wait_for_send_pkg_ptr a_receive_reply, bool& a_chan_valid, int a_timeout_ms )
    {
        std::unique_lock< std::mutex > t_lock( a_receive_reply->f_mutex );
        auto t_deadline = std::chrono::system_clock::now() + std::chrono::milliseconds( a_timeout_ms );
        while( ! a_receive_reply->f_sent_msg_pkg_ptr )
        {
            std::cv_status t_status = a_receive_reply->f_condition_var.wait_until( t_lock, t_deadline );
            if( t_status == std::cv_status::timeout )
            {
                // timeout
                return reply_ptr_t();
            }
        }
        return core::wait_for_reply( a_receive_reply->f_sent_msg_pkg_ptr, a_chan_valid, a_timeout_ms );
    }

}
