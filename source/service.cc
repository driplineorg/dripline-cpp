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

    service::service() :
            f_address(),
            f_port(),
            f_username( "guest" ),
            f_password( "guest" ),
            f_exchange(),
            f_queue_name(),
            f_channel(),
            f_consumer_tag(),
            f_keys(),
            f_broadcast_key( "broadcast" ),
            f_canceled( false )
    {}

    service::service( const string& a_address, unsigned a_port, const string& a_exchange, const string& a_queue_name, const string& a_auth_file ) :
            f_address( a_address ),
            f_port( a_port ),
            f_username( "guest" ),
            f_password( "guest" ),
            f_exchange( a_exchange ),
            f_queue_name( a_queue_name ),
            f_channel(),
            f_consumer_tag(),
            f_keys(),
            f_broadcast_key( "broadcast.#" ),
            f_canceled( false )
    {
        if( ! a_auth_file.empty() )
        {
            if( ! use_auth_file( a_auth_file ) )
            {
                throw scarab::error() << "An error occurred while using authorization file <" << a_auth_file << ">";
            }
        }
    }

    service::~service()
    {
    }

    bool service::start()
    {
        if( f_queue_name.empty() )
        {
            LWARN( dlog, "Service requires a queue name to be started" );
            return false;
        }

        LINFO( dlog, "Connecting to <" << f_address << ":" << f_port << ">" );

        f_channel = open_channel();
        if( ! f_channel ) return false;

        if( ! setup_exchange( f_channel, f_exchange ) ) return false;

        if( ! setup_queue( f_channel, f_queue_name ) ) return false;

        if( ! bind_keys( f_keys ) ) return false;

        if( ! start_consuming() ) return false;

        f_canceled.store( false );

        return true;
    }

    bool service::listen( int a_timeout_ms )
    {
        LINFO( dlog, "Listening for incoming messages on <" << f_queue_name << ">" );

        while( ! f_canceled.load()  )
        {
            amqp_envelope_ptr t_envelope;
            bool t_channel_valid = listen_for_message( t_envelope, f_channel, f_consumer_tag, a_timeout_ms );

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
                    send( t_reply );
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


    service::rr_pkg_ptr service::send( request_ptr_t a_request, const string& a_exchange ) const
    {
        LDEBUG( dlog, "Sending request with routing key <" << a_request->routing_key() << ">" );
        rr_pkg_ptr t_receive_reply = std::make_shared< receive_reply_pkg >();
        t_receive_reply->f_channel = send_withreply( static_pointer_cast< message >( a_request ), t_receive_reply->f_consumer_tag, a_exchange );
        t_receive_reply->f_successful_send = t_receive_reply->f_channel.get() != nullptr;
        return t_receive_reply;
    }

    bool service::send( reply_ptr_t a_reply, const string& a_exchange ) const
    {
        LDEBUG( dlog, "Sending reply with routing key <" << a_reply->routing_key() << ">" );
        return send_noreply( static_pointer_cast< message >( a_reply ), a_exchange );
    }

    bool service::send( alert_ptr_t a_alert, const string& a_exchange ) const
    {
        LDEBUG( dlog, "Sending alert with routing key <" << a_alert->routing_key() << ">" );
        return send_noreply( static_pointer_cast< message >( a_alert ), a_exchange );
    }

    reply_ptr_t service::wait_for_reply( const rr_pkg_ptr a_receive_reply, int a_timeout_ms ) const
    {
        bool t_temp;
        return wait_for_reply( a_receive_reply, t_temp, a_timeout_ms );
    }

    reply_ptr_t service::wait_for_reply( const rr_pkg_ptr a_receive_reply, bool& a_chan_valid, int a_timeout_ms ) const
    {
        LDEBUG( dlog, "Waiting for a reply" );

        amqp_envelope_ptr t_envelope;
        a_chan_valid = listen_for_message( t_envelope, a_receive_reply->f_channel, a_receive_reply->f_consumer_tag, a_timeout_ms );

        try
        {
            message_ptr_t t_message = message::process_envelope( t_envelope );

            if( t_message->is_reply() )
            {
                return static_pointer_cast< msg_reply >( t_message );
            }
            else
            {
                LERROR( dlog, "Non-reply message received");
                return reply_ptr_t();
            }
        }
        catch( dripline_error& e )
        {
            LERROR( dlog, "There was a problem processing the message: " << e.what() );
            return reply_ptr_t();
        }
    }

    bool service::close_channel( amqp_channel_ptr a_channel ) const
    {
        try
        {
            LDEBUG( dlog, "Stopping consuming messages" );
            f_channel->BasicCancel( f_consumer_tag );
        }
        catch( amqp_exception& e )
        {
            LERROR( dlog, "AMQP exception caught while canceling the channel: (" << e.reply_code() << ") " << e.reply_text() );
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "AMQP library exception caught while canceling the channel: (" << e.ErrorCode() << ") " << e.what() );
        }

        a_channel.reset();
        return true;
    }


    bool service::on_request_message( const request_ptr_t )
    {
        throw dripline_error() << retcode_t::message_error_invalid_method << "Base service does not handle request messages";
        return false;
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


    amqp_channel_ptr service::send_withreply( message_ptr_t a_message, string& a_reply_consumer_tag, const string& a_exchange ) const
    {
        a_message->set_sender_service_name( f_queue_name );

        string t_exchange = a_exchange;
        if( t_exchange.empty() )
        {
            t_exchange = f_exchange;
        }

        amqp_channel_ptr t_channel = open_channel();
        if( ! t_channel )
        {
            LERROR( dlog, "Unable to open channel to send a message to <" << a_message->routing_key() << "> using broker <" << f_address << ":" << f_port << ">" );
            return amqp_channel_ptr();
        }

        if( ! setup_exchange( t_channel, t_exchange ) )
        {
            LERROR( dlog, "Unable to setup the exchange <" << t_exchange << ">" );
            return amqp_channel_ptr();
        }

        // create the reply-to queue, and bind the queue to the routing key over the given exchange
        string t_reply_to = t_channel->DeclareQueue( "" );
        t_channel->BindQueue( t_reply_to, t_exchange, t_reply_to );
        a_message->reply_to() = t_reply_to;

        // begin consuming on the reply-to queue
        a_reply_consumer_tag = t_channel->BasicConsume( t_reply_to );
        LDEBUG( dlog, "Reply-to for request: " << t_reply_to );
        LDEBUG( dlog, "Consumer tag for reply: " << a_reply_consumer_tag );

        // convert the dripline::message object to an AMQP message
        amqp_message_ptr t_amqp_message = a_message->create_amqp_message();

        try
        {
            t_channel->BasicPublish( t_exchange, a_message->routing_key(), t_amqp_message, true, false );
            LDEBUG( dlog, "Message sent" );
            return t_channel;
        }
        catch( AmqpClient::MessageReturnedException& e )
        {
            LERROR( dlog, "Message could not be sent: " << e.what() );
            return amqp_channel_ptr();
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Error publishing request to queue: " << e.what() );
            return amqp_channel_ptr();
        }
    }

    bool service::send_noreply( message_ptr_t a_message, const string& a_exchange ) const
    {
        a_message->set_sender_service_name( f_queue_name );

        string t_exchange = a_exchange;
        if( t_exchange.empty() )
        {
            t_exchange = f_exchange;
        }

        amqp_channel_ptr t_channel = open_channel();
        if( ! t_channel )
        {
            LERROR( dlog, "Unable to open channel to send a message to <" << a_message->routing_key() << "> using broker <" << f_address << ":" << f_port << ">" );
            return false;
        }

        if( ! setup_exchange( t_channel, t_exchange ) )
        {
            LERROR( dlog, "Unable to setup the exchange <" << t_exchange << ">" );
            return false;
        }

        amqp_message_ptr t_amqp_message = a_message->create_amqp_message();

        try
        {
            t_channel->BasicPublish( t_exchange, a_message->routing_key(), t_amqp_message, true, false );
            LDEBUG( dlog, "Message sent" );
        }
        catch( AmqpClient::MessageReturnedException& e )
        {
            LERROR( dlog, "Message could not be sent: " << e.what() );
            return false;
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Error publishing request to queue: " << e.what() );
            return false;
        }
        return true;
    }

    amqp_channel_ptr service::open_channel() const
    {
        try
        {
            LDEBUG( dlog, "Opening AMQP connection and creating channel to " << f_address << ":" << f_port );
            LDEBUG( dlog, "Using broker authentication: " << f_username << ":" << f_password );
            return AmqpClient::Channel::Create( f_address, f_port, f_username, f_password );
        }
        catch( amqp_exception& e )
        {
            LERROR( dlog, "AMQP exception caught while opening channel: (" << e.reply_code() << ") " << e.reply_text() );
            return amqp_channel_ptr();
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "AMQP Library Exception caught while creating channel: (" << e.ErrorCode() << ") " << e.what() );
            return amqp_channel_ptr();
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Standard exception caught while creating channel: " << e.what() );
            return amqp_channel_ptr();
        }
    }

    bool service::setup_exchange( amqp_channel_ptr a_channel, const string& a_exchange ) const
    {
        try
        {
            LDEBUG( dlog, "Declaring exchange <" << a_exchange << ">" );
            a_channel->DeclareExchange( a_exchange, AmqpClient::Channel::EXCHANGE_TYPE_TOPIC, false, false, false );
            return true;
        }
        catch( amqp_exception& e )
        {
            LERROR( dlog, "AMQP exception caught while declaring exchange: (" << e.reply_code() << ") " << e.reply_text() );
            return false;
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "AMQP library exception caught while declaring exchange: (" << e.ErrorCode() << ") " << e.what() );
            return false;
        }
    }

    bool service::setup_queue( amqp_channel_ptr a_channel, const string& a_queue_name ) const
    {
        try
        {
            LDEBUG( dlog, "Declaring queue <" << a_queue_name << ">" );
            a_channel->DeclareQueue( a_queue_name, false, false, true, true );
            return true;
        }
        catch( amqp_exception& e )
        {
            LERROR( dlog, "AMQP exception caught while declaring queue: (" << e.reply_code() << ") " << e.reply_text() );
            return false;
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "AMQP library exception caught while declaring queue: (" << e.ErrorCode() << ") " << e.what() );
            return false;
        }

    }

    bool service::bind_keys( const set< string >& a_keys )
    {
        try
        {
            for( set< string >::const_iterator t_key_it = a_keys.begin(); t_key_it != a_keys.end(); ++t_key_it )
            {
                f_channel->BindQueue( f_queue_name, f_exchange, *t_key_it );
            }
            f_channel->BindQueue( f_queue_name, f_exchange, f_broadcast_key + ".#" );
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

    bool service::listen_for_message( amqp_envelope_ptr& a_envelope, amqp_channel_ptr a_channel, const string& a_consumer_tag, int a_timeout_ms ) const
    {
        while( true )
        {
            try
            {
                if( a_timeout_ms > 0 )
                {
                    a_channel->BasicConsumeMessage( a_consumer_tag, a_envelope, a_timeout_ms );
                }
                else
                {
                    a_envelope = a_channel->BasicConsumeMessage( a_consumer_tag );
                }
                if( a_envelope ) a_channel->BasicAck( a_envelope );
                return true;
            }
            catch( AmqpClient::ConnectionClosedException& e )
            {
                LERROR( dlog, "Fatal AMQP exception encountered: " << e.what() );
                return false;
            }
            catch( AmqpClient::ConsumerCancelledException& e )
            {
                LERROR( dlog, "Fatal AMQP exception encountered: " << e.what() );
                return false;
            }
            catch( AmqpClient::AmqpException& e )
            {
                if( e.is_soft_error() )
                {
                    LWARN( dlog, "Non-fatal AMQP exception encountered: " << e.reply_text() );
                    return true;
                }
                LERROR( dlog, "Fatal AMQP exception encountered: " << e.reply_text() );
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
    }

    bool service::stop_consuming()
    {
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

    bool service::use_auth_file( const string& a_auth_file )
    {
        authentication t_auth( a_auth_file );
        if( ! t_auth.get_is_loaded() )
        {
            LERROR( dlog, "Authentication file <" << a_auth_file << "> could not be loaded" );
            return false;
        }

        const param_node* t_amqp_auth = t_auth.node_at( "amqp" );
        if( t_amqp_auth == NULL || ! t_amqp_auth->has( "username" ) || ! t_amqp_auth->has( "password" ) )
        {
            LERROR( dlog, "AMQP authentication is not available or is not complete" );
            return false;
        }
        f_username = t_amqp_auth->get_value( "username" );
        f_password = t_amqp_auth->get_value( "password" );
        return true;
    }

} /* namespace dripline */
