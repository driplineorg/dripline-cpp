/*
 * core.cc
 *
 *  Created on: Jun 27, 2017
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "core.hh"

#include "dripline_error.hh"
#include "message.hh"

#include "authentication.hh"
#include "logger.hh"


namespace dripline
{

    LOGGER( dlog, "amqp" );

    sent_msg_pkg::~sent_msg_pkg()
    {
        if( f_channel )
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
        }
    }

    bool core::s_offline = false;

    core::core( const scarab::param_node& a_config, const std::string& a_broker_address, unsigned a_port, const std::string& a_auth_file, const bool a_make_connection ) :
            f_address( "localhost" ),
            f_port( 5672 ),
            f_username( "guest" ),
            f_password( "guest" ),
            f_requests_exchange( "requests" ),
            f_alerts_exchange( "alerts" ),
            f_heartbeat_routing_key( "heartbeat" ),
            f_max_payload_size( DL_MAX_PAYLOAD_SIZE ),
            f_make_connection( a_make_connection )
    {
        // auth file passed as a parameter overrides a file passed in the config
        std::string t_auth_file( a_auth_file );
        if( t_auth_file.empty() ) t_auth_file = a_config.get_value( "auth-file", "" );

        // get auth file contents and override defaults
        if( ! t_auth_file.empty() )
        {
            LDEBUG( dlog, "Using authentication file <" << t_auth_file << ">" );

            scarab::authentication t_auth( t_auth_file );
            if( ! t_auth.get_is_loaded() )
            {
                throw dripline_error() << "Authentication file <" << a_auth_file << "> could not be loaded";
            }

            if( ! t_auth.has( "amqp" ) )
            {
                throw dripline_error() << "No \"amqp\" authentication information present in <" << a_auth_file << ">";
            }

            const scarab::param_node& t_amqp_auth = t_auth["amqp"].as_node();
            if( ! t_amqp_auth.has( "username" ) || ! t_amqp_auth.has( "password" ) )
            {
                throw dripline_error() <<  "AMQP authentication is not available or is not complete";
            }
            f_username = t_amqp_auth["username"]().as_string();
            f_password = t_amqp_auth["password"]().as_string();

            if( t_amqp_auth.has( "broker" ) )
            {
                f_address = t_amqp_auth["broker"]().as_string();
            }
        }

        // config file overrides auth file and defaults
        if( ! a_config.empty() )
        {
            f_address = a_config.get_value( "broker", f_address );
            f_port = a_config.get_value( "broker-port", f_port );
            f_requests_exchange = a_config.get_value( "requests-exchange", f_requests_exchange );
            f_alerts_exchange = a_config.get_value( "alerts-exchange", f_alerts_exchange );
            f_heartbeat_routing_key = a_config.get_value( "heartbeat-routing-key", f_heartbeat_routing_key );
            f_max_payload_size = a_config.get_value( "max-payload-size", f_max_payload_size );
            f_make_connection = a_config.get_value( "make-connection", f_make_connection );
        }

        // parameters override config file, auth file, and defaults
        if( ! a_broker_address.empty() ) f_address = a_broker_address;
        if( a_port != 0 ) f_port = a_port;
    }

    core::core( const bool a_make_connection, const scarab::param_node& a_config ) :
            core::core( a_config )
    {
        // this constructor overrides the default value of make_connection
        f_make_connection = a_make_connection;
    }

    core::core( const core& a_orig ) :
            f_address( a_orig.f_address ),
            f_port( a_orig.f_port ),
            f_username( a_orig.f_username ),
            f_password( a_orig.f_password ),
            f_requests_exchange( a_orig.f_requests_exchange ),
            f_alerts_exchange( a_orig.f_alerts_exchange ),
            f_heartbeat_routing_key( a_orig.f_heartbeat_routing_key ),
            f_max_payload_size( a_orig.f_max_payload_size ),
            f_make_connection( a_orig.f_make_connection )
    {}

    core::core( core&& a_orig ) :
            f_address( std::move(a_orig.f_address) ),
            f_port( a_orig.f_port ),
            f_username( std::move(a_orig.f_username) ),
            f_password( std::move(a_orig.f_password) ),
            f_requests_exchange( std::move(a_orig.f_requests_exchange) ),
            f_alerts_exchange( std::move(a_orig.f_alerts_exchange) ),
            f_heartbeat_routing_key( std::move(a_orig.f_heartbeat_routing_key) ),
            f_max_payload_size( a_orig.f_max_payload_size ),
            f_make_connection( std::move(a_orig.f_make_connection) )
    {
        a_orig.f_port = 0;
        a_orig.f_max_payload_size = DL_MAX_PAYLOAD_SIZE;
    }

    core::~core()
    {}

    core& core::operator=( const core& a_orig )
    {
        f_address = a_orig.f_address;
        f_port = a_orig.f_port;
        f_username = a_orig.f_username;
        f_password = a_orig.f_password;
        f_requests_exchange = a_orig.f_requests_exchange;
        f_alerts_exchange = a_orig.f_alerts_exchange;
        f_heartbeat_routing_key = a_orig.f_heartbeat_routing_key;
        f_max_payload_size = a_orig.f_max_payload_size;
        f_make_connection = a_orig.f_make_connection;
        return *this;
    }

    core& core::operator=( core&& a_orig )
    {
        f_address = std::move( a_orig.f_address );
        f_port = a_orig.f_port;
        a_orig.f_port = 0;
        f_username = std::move( a_orig.f_username );
        f_password = std::move( a_orig.f_password );
        f_requests_exchange = std::move( a_orig.f_requests_exchange );
        f_alerts_exchange = std::move( a_orig.f_alerts_exchange );
        f_heartbeat_routing_key = std::move( a_orig.f_heartbeat_routing_key );
        f_max_payload_size = a_orig.f_max_payload_size;
        a_orig.f_max_payload_size = DL_MAX_PAYLOAD_SIZE;
        f_make_connection = std::move( a_orig.f_make_connection );
        return *this;
    }

    sent_msg_pkg_ptr core::send( request_ptr_t a_request ) const
    {
        LDEBUG( dlog, "Sending request with routing key <" << a_request->routing_key() << ">" );
        return do_send( std::static_pointer_cast< message >( a_request ), f_requests_exchange, true );
    }

    sent_msg_pkg_ptr core::send( reply_ptr_t a_reply ) const
    {
        LDEBUG( dlog, "Sending reply with routing key <" << a_reply->routing_key() << ">" );
        return do_send( std::static_pointer_cast< message >( a_reply ), f_requests_exchange, false );
    }

    sent_msg_pkg_ptr core::send( alert_ptr_t a_alert ) const
    {
        LDEBUG( dlog, "Sending alert with routing key <" << a_alert->routing_key() << ">" );
        return do_send( std::static_pointer_cast< message >( a_alert ), f_alerts_exchange, false );
    }

    sent_msg_pkg_ptr core::do_send( message_ptr_t a_message, const std::string& a_exchange, bool a_expect_reply ) const
    {
        // throws dripline_error if it could not start sending the message
        // returns the receive_reply package if the message was completely or partially sent
        // the f_successful_send flag will be set accordingly: true if completely sent; false if partially sent
        // if there was an error, that will be returned in f_send_error_message, which will be empty otherwise

        if ( ! f_make_connection || core::s_offline )
        {
            throw a_message;
            //throw dripline_error() << "cannot send reply when make_connection is false";
        }

        amqp_channel_ptr t_channel = open_channel();
        if( ! t_channel )
        {
            throw dripline_error() << "Unable to open channel to send a message to <" << a_message->routing_key() << "> using broker <" << f_address << ":" << f_port << ">";
        }

        if( ! setup_exchange( t_channel, a_exchange ) )
        {
            throw dripline_error() << "Unable to setup the exchange <" << a_exchange << ">";
        }

        // create empty receive-reply object
        sent_msg_pkg_ptr t_receive_reply = std::make_shared< sent_msg_pkg >();
        std::unique_lock< std::mutex > t_rr_lock( t_receive_reply->f_mutex );

        if( a_expect_reply )
        {
            t_receive_reply->f_channel = t_channel;

            // create the reply-to queue, and bind the queue to the routing key over the given exchange
            std::string t_reply_to = t_channel->DeclareQueue( "" );
            t_channel->BindQueue( t_reply_to, a_exchange, t_reply_to );
            // set the reply-to in the message because now we have the queue to which to reply
            a_message->reply_to() = t_reply_to;

            // begin consuming on the reply-to queue
            t_receive_reply->f_consumer_tag = t_channel->BasicConsume( t_reply_to );
            LDEBUG( dlog, "Reply-to for request: " << t_reply_to );
            LDEBUG( dlog, "Consumer tag for reply: " << t_receive_reply->f_consumer_tag );
        }

        // convert the dripline::message object to an AMQP message
        amqp_split_message_ptrs t_amqp_messages = a_message->create_amqp_messages( f_max_payload_size );
        if( t_amqp_messages.empty() )
        {
            throw dripline_error() << "Unable to convert the dripline::message object to AMQP messages";
        }

        try
        {
            LDEBUG( dlog, "Sending message to <" << a_message->routing_key() << ">" );
            for( amqp_message_ptr& t_amqp_message : t_amqp_messages )
            {
                t_channel->BasicPublish( a_exchange, a_message->routing_key(), t_amqp_message, true, false );
            }
            LDEBUG( dlog, "Message sent in " << t_amqp_messages.size() << " chunks" );
            t_receive_reply->f_successful_send = true;
            t_receive_reply->f_send_error_message.clear();
        }
        catch( AmqpClient::MessageReturnedException& e )
        {
            LERROR( dlog, "Message could not be sent: " << e.what() );
            t_receive_reply->f_successful_send = false;
            t_receive_reply->f_send_error_message = std::string("AMQP error while sending message: ") + std::string(e.what());
        }
        catch( std::exception& e )
        {
            t_receive_reply->f_successful_send = false;
            t_receive_reply->f_send_error_message = std::string("Error publishing request to queue: ") + std::string(e.what());
        }

        return t_receive_reply;
    }

    amqp_channel_ptr core::open_channel() const
    {
        if ( ! f_make_connection || core::s_offline )
        {
            return amqp_channel_ptr();
            //throw dripline_error() << "Should not call open_channel when offline";
        }
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

    bool core::setup_exchange( amqp_channel_ptr a_channel, const std::string& a_exchange )
    {
        if( s_offline || ! a_channel )
        {
            return false;
        }

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

    bool core::setup_queue( amqp_channel_ptr a_channel, const std::string& a_queue_name )
    {
        if( s_offline || ! a_channel )
        {
            return false;
        }

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

    bool core::listen_for_message( amqp_envelope_ptr& a_envelope, amqp_channel_ptr a_channel, const std::string& a_consumer_tag, int a_timeout_ms, bool a_do_ack )
    {
        if( s_offline || ! a_channel )
        {
            return false;
        }

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
                if( a_envelope && a_do_ack ) a_channel->BasicAck( a_envelope );
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

} /* namespace dripline */

