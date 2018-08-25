#define DRIPLINE_API_EXPORTS

#include "relayer.hh"

#include "dripline_error.hh"

#include "logger.hh"
#include "param.hh"

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

            switch( t_mar->f_message->message_type() )
            {
                case msg_t::request:
                { // add scope for controling the lock's existence
                    std::unique_lock< std::mutex > lock( t_mar->f_receive_reply->f_mutex );
                    *t_mar->f_receive_reply = *core::send( std::static_pointer_cast< dripline::msg_request >( t_mar->f_message ) );
                    if( ! t_mar->f_receive_reply->f_successful_send )
                    {
                        LWARN( dlog, "Unable to send request" );
                    }
                    t_mar->f_receive_reply->f_condition_var.notify_one();
                    break;
                }
                case msg_t::alert:
                    if( ! core::send( std::static_pointer_cast< dripline::msg_alert >( t_mar->f_message ) ) )
                    {
                        LWARN( dlog, "Unable to send alert" );
                    }
                    break;
                default:
                    LWARN( dlog, "Unsupported message type: " << t_mar->f_message->message_type() );
                    break;
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

    relayer::cc_rr_pkg_ptr relayer::send_async( request_ptr_t a_request ) const
    {
        if( is_canceled() )
        {
            LWARN( dlog, "Relayer has been canceled; request not sent" );
            cc_rr_pkg_ptr t_return;
            t_return->f_successful_send = false;
            return t_return;
        }
        LDEBUG( dlog, "Sending request to <" << a_request->routing_key() << ">" );
        mar_ptr t_mar = std::make_shared< message_and_reply >();
        std::unique_lock< std::mutex > lock( t_mar->f_receive_reply->f_mutex );
        t_mar->f_message = std::static_pointer_cast< dripline::message >( a_request );
        t_mar->f_receive_reply = std::make_shared< cc_receive_reply_pkg >();
        f_queue.push( t_mar );
        return t_mar->f_receive_reply;
    }

    bool relayer::send_async( alert_ptr_t a_alert ) const
    {
        if( is_canceled() )
        {
            LWARN( dlog, "Relayer has been canceled; request not sent" );
            return false;
        }
        LDEBUG( dlog, "Sending request to <" << a_alert->routing_key() << ">" );
        mar_ptr t_mar = std::make_shared< message_and_reply >();
        t_mar->f_message = std::static_pointer_cast< dripline::message >( a_alert );
        f_queue.push( t_mar );
        return true;
    }

    reply_ptr_t relayer::wait_for_reply( const cc_rr_pkg_ptr a_receive_reply, int a_timeout_ms )
    {
        bool t_temp;
        return wait_for_reply( a_receive_reply, t_temp, a_timeout_ms );
    }

    reply_ptr_t relayer::wait_for_reply( const cc_rr_pkg_ptr a_receive_reply, bool& a_chan_valid, int a_timeout_ms )
    {
        std::unique_lock< std::mutex > lock( a_receive_reply->f_mutex );
        // TODO: wait on condition (timed?)
        return core::wait_for_reply( std::static_pointer_cast< receive_reply_pkg >( a_receive_reply ), a_chan_valid, a_timeout_ms );
    }

}
