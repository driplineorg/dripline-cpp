/*
 * service.cc
 *
 *  Created on: Jan 5, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "service.hh"

#include "dripline_error.hh"

#include "authentication.hh"
#include "logger.hh"

using scarab::authentication;
using scarab::param_node;
using scarab::param_value;
using scarab::parsable;

using std::static_pointer_cast;
using std::string;
using std::set;

namespace dripline
{
    LOGGER( dlog, "service" );

    service::service( const scarab::param_node* a_config, const string& a_queue_name,  const std::string& a_broker_address, unsigned a_port, const std::string& a_auth_file, const bool a_make_connection ) :
            core( a_config, a_broker_address, a_port, a_auth_file, a_make_connection ),
            f_queue_name( "dlcpp_service" ),
            f_channel(),
            f_consumer_tag(),
            f_keys(),
            f_broadcast_key( "broadcast" ),
            f_listen_timeout_ms( 500 ),
            f_canceled( false )
    {
        if( a_config != nullptr )
        {
            f_queue_name = a_config->get_value( "queue", f_queue_name );
            f_listen_timeout_ms = a_config->get_value( "listen-timeout-ms", f_listen_timeout_ms );
        }

        if( ! a_queue_name.empty() ) f_queue_name = a_queue_name;
    }

    service::service( const bool a_make_connection, const scarab::param_node* a_config ) :
            core( a_make_connection, a_config ),
            f_queue_name( "" ),
            f_channel(),
            f_consumer_tag(),
            f_keys(),
            f_broadcast_key(),
            f_listen_timeout_ms( 500 ),
            f_canceled( false )
    {
    }

    service::~service()
    {
    }

    rr_pkg_ptr service::send( request_ptr_t a_request ) const
    {
        a_request->set_sender_service_name( f_queue_name );
        return core::send( a_request );
    }

    bool service::send( reply_ptr_t a_reply ) const
    {
        a_reply->set_sender_service_name( f_queue_name );
        return core::send( a_reply );
    }

    bool service::send( alert_ptr_t a_alert ) const
    {
        a_alert->set_sender_service_name( f_queue_name );
        return core::send( a_alert );
    }

    bool service::start()
    {
        if( ! f_make_connection )
        {
            LWARN( dlog, "Should not start service when make_connection is disabled" );
            return true;
        }
        if( f_queue_name.empty() )
        {
            LERROR( dlog, "Service requires a queue name to be started" );
            return false;
        }

        LINFO( dlog, "Connecting to <" << f_address << ":" << f_port << ">" );

        f_channel = open_channel();
        if( ! f_channel ) return false;

        if( ! setup_exchange( f_channel, f_requests_exchange ) ) return false;

        if( ! setup_queue( f_channel, f_queue_name ) ) return false;

        if( ! bind_keys( f_keys ) ) return false;

        if( ! start_consuming() ) return false;

        f_canceled.store( false );

        return true;
    }

    bool service::listen()
    {
        LINFO( dlog, "Listening for incoming messages on <" << f_queue_name << ">" );
        //TODO
        if ( ! f_make_connection )
        {
            return true;
        }
        while( ! f_canceled.load()  )
        {

            amqp_envelope_ptr t_envelope;
            bool t_channel_valid = listen_for_message( t_envelope, f_channel, f_consumer_tag, f_listen_timeout_ms );

            if( f_canceled.load() )
            {
                LDEBUG( dlog, "Service canceled" );
                return true;
            }

            if( ! t_envelope && t_channel_valid )
            {
                continue;
            }

            try
            {
                message_ptr_t t_message = message::process_envelope( t_envelope );

                bool t_msg_handled = true;
                if( t_message->is_request() )
                {
                    if( ! set_routing_key_specifier( t_message ) )
                    {
                        throw dripline_error() << retcode_t::message_error_decoding_fail << "Unable to determine the routing-key specifier; routing key: <" << t_message->routing_key() << ">";
                    }

                    t_msg_handled = on_request_message( static_pointer_cast< msg_request >( t_message ) );
                }
                else if( t_message->is_alert() )
                {
                    t_msg_handled = on_alert_message( static_pointer_cast< msg_alert >( t_message ) );
                }
                else if( t_message->is_reply() )
                {
                    t_msg_handled = on_reply_message( static_pointer_cast< msg_reply >( t_message ) );
                }
                if( ! t_msg_handled )
                {
                    throw dripline_error() << retcode_t::message_error << "Message could not be handled";
                }
            }
            catch( dripline_error& e )
            {
                reply_ptr_t t_reply = msg_reply::create( e, t_envelope->Message()->ReplyTo(), message::encoding::json );
                try
                {
                    core::send( t_reply );
                }
                catch( amqp_exception& e )
                {
                    LERROR( dlog, "AMQP exception caught while sending reply: (" << e.reply_code() << ") " << e.reply_text() );
                }
                catch( amqp_lib_exception& e )
                {
                    LERROR( dlog, "AMQP Library Exception caught while sending reply: (" << e.ErrorCode() << ") " << e.what() );
                }
                catch( std::exception& e )
                {
                    LERROR( dlog, "Standard exception caught while sending reply: " << e.what() );
                }
            }

            if( ! t_channel_valid )
            {
                LERROR( dlog, "Channel is no longer valid" );
                return false;
            }

            if( f_canceled.load() )
            {
                LDEBUG( dlog, "Service canceled" );
                return true;
            }
        }
        return true;
    }

    bool service::stop()
    {
        LINFO( dlog, "Stopping service on <" << f_queue_name << ">" );

        if( ! stop_consuming() ) return false;

        if( ! remove_queue() ) return false;

        return true;
    }


    reply_info service::submit_request_message( const request_ptr_t a_request_ptr)
    {
        return this->on_request_message( a_request_ptr );;
    }

    bool service::submit_alert_message( const alert_ptr_t a_alert_ptr)
    {
        return this->on_alert_message( a_alert_ptr );
    }

    bool service::submit_reply_message( const reply_ptr_t a_reply_ptr)
    {
        return this->on_reply_message( a_reply_ptr );
    }

    reply_info service::on_request_message( const request_ptr_t )
    {
        throw dripline_error() << retcode_t::message_error_invalid_method << "Base service does not handle request messages";
        return reply_info( false, retcode_t::message_error_invalid_method, "" );
    }

    bool service::on_reply_message( const reply_ptr_t )
    {
        throw dripline_error() << retcode_t::message_error_invalid_method << "Base service does not handle reply messages";
        return false;
    }

    bool service::on_alert_message( const alert_ptr_t )
    {
        throw dripline_error() << retcode_t::message_error_invalid_method << "Base service does not handle alert messages";
        return false;
    }


    bool service::set_routing_key_specifier( message_ptr_t a_message ) const
    {
        string t_rk( a_message->routing_key() );
        string t_prefix;
        if( t_rk.find( f_queue_name ) == 0 ) t_prefix = f_queue_name;
        else if( t_rk.find( f_broadcast_key ) == 0 ) t_prefix = f_broadcast_key;
        else
        {
            LWARN( dlog, "Routing key not formatted properly; it does not start with either the queue name (" << f_queue_name << ") or the broadcast key (" << f_broadcast_key << "): <" << t_rk << ">" );
            return false;
        }

        if( t_rk == t_prefix )
        {
            // rk consists of only the prefix
            a_message->set_routing_key_specifier( "", routing_key_specifier() );
            return true;
        }

        if( t_rk[ t_prefix.size() ] != '.' )
        {
            LWARN( dlog, "Routing key not formatted properly; a single '.' does not follow the prefix: <" << t_rk << ">" );
            return false;
        }

        t_rk.erase( 0, t_prefix.size() + 1 ); // 1 added to remove the '.' that separates nodes
        a_message->set_routing_key_specifier( t_rk, routing_key_specifier( t_rk ) );
        LDEBUG( dlog, "Determined the RKS to be <" << a_message->parsed_rks().to_string() << ">; size = " << a_message->parsed_rks().size() );
        return true;
    }


    bool service::bind_keys( const set< string >& a_keys )
    {
        try
        {
            for( set< string >::const_iterator t_key_it = a_keys.begin(); t_key_it != a_keys.end(); ++t_key_it )
            {
                f_channel->BindQueue( f_queue_name, f_requests_exchange, *t_key_it );
            }
            f_channel->BindQueue( f_queue_name, f_requests_exchange, f_broadcast_key + ".#" );
            return true;
        }
        catch( amqp_exception& e )
        {
            LERROR( dlog, "AMQP exception caught while declaring binding keys: (" << e.reply_code() << ") " << e.reply_text() );
            return false;
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "AMQP library exception caught while binding keys: (" << e.ErrorCode() << ") " << e.what() );
            return false;
        }
    }

    bool service::start_consuming()
    {
        try
        {
            LDEBUG( dlog, "Starting to consume messages" );
            // second bool is setting no_ack to false
            f_consumer_tag = f_channel->BasicConsume( f_queue_name, "", true, false );
            return true;
        }
        catch( amqp_exception& e )
        {
            LERROR( dlog, "AMQP exception caught while starting consuming messages: (" << e.reply_code() << ") " << e.reply_text() );
            return false;
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "AMQP library exception caught while starting consuming messages: (" << e.ErrorCode() << ") " << e.what() );
            return false;
        }
    }

    bool service::stop_consuming()
    {
        if ( ! f_make_connection )
        {
            LDEBUG( dlog, "no consuming to start because connections disabled" );
            return true;
        }
        try
        {
            LDEBUG( dlog, "Stopping consuming messages (consumer " << f_consumer_tag << ")" );
            f_channel->BasicCancel( f_consumer_tag );
            f_consumer_tag.clear();
            return true;
        }
        catch( amqp_exception& e )
        {
            LERROR( dlog, "AMQP exception caught while stopping consuming messages: (" << e.reply_code() << ") " << e.reply_text() );
            return false;
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "AMQP library exception caught while stopping consuming messages: (" << e.ErrorCode() << ") " << e.what() );
            return false;
        }
        catch( AmqpClient::ConsumerTagNotFoundException& e )
        {
            LERROR( dlog, "Fatal AMQP exception encountered: " << e.what() );
            return false;
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Standard exception caught: " << e.what() );
            return false;
        }
        catch(...)
        {
            LERROR( dlog, "Unknown exception caught" );
            return false;
        }
    }

    bool service::remove_queue()
    {
        if ( ! f_make_connection )
        {
            LDEBUG( dlog, "no queue to remove because make_connection is false" );
            return true;
        }
        try
        {
            LDEBUG( dlog, "Deleting queue <" << f_queue_name << ">" );
            f_channel->DeleteQueue( f_queue_name, false );
            f_queue_name.clear();
        }
        catch( AmqpClient::ConnectionClosedException& e )
        {
            LERROR( dlog, "Fatal AMQP exception encountered: " << e.what() );
            return false;
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "AMQP library exception caught while removing queue: (" << e.ErrorCode() << ") " << e.what() );
            return false;
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Standard exception caught: " << e.what() );
            return false;
        }
        catch(...)
        {
            LERROR( dlog, "Unknown exception caught" );
            return false;
        }

        return true;
    }

} /* namespace dripline */
