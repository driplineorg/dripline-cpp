/*
 * amqp.cc
 *
 *  Created on: Jun 27, 2017
 *      Author: obla999
 */


#include "core.hh"

#include "dripline_error.hh"
#include "message.hh"

#include "authentication.hh"
#include "logger.hh"


namespace dripline
{

    LOGGER( dlog, "amqp" );

    receive_reply_pkg::~receive_reply_pkg()
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
            f_channel.reset();
        }
    }

    core::core( const scarab::param_node* a_config, const std::string& a_broker_address, unsigned a_port, const std::string& a_auth_file ) :
            f_address( "localhost" ),
            f_port( 5672 ),
            f_username( "guest" ),
            f_password( "guest" ),
            f_requests_exchange( "requests" ),
            f_alerts_exchange( "alerts" ),
            f_make_connection( true )
    {
        std::string t_auth_file = a_config->get_value( "auth-file", a_auth_file );

        // get auth file contents and override defaults
        if( ! t_auth_file.empty() )
        {
            LDEBUG( dlog, "Using authentication file <" << t_auth_file << ">" );

            scarab::authentication t_auth( t_auth_file );
            if( ! t_auth.get_is_loaded() )
            {
                throw dripline_error() << "Authentication file <" << a_auth_file << "> could not be loaded";
            }

            const scarab::param_node* t_amqp_auth = t_auth.node_at( "amqp" );
            if( t_amqp_auth == NULL || ! t_amqp_auth->has( "username" ) || ! t_amqp_auth->has( "password" ) )
            {
                throw dripline_error() <<  "AMQP authentication is not available or is not complete";
            }
            f_username = t_amqp_auth->get_value( "username" );
            f_password = t_amqp_auth->get_value( "password" );

            if( f_address.empty() && t_amqp_auth->has( "broker" ) )
            {
                f_address = t_amqp_auth->get_value( "broker" );
            }
        }

        // config file overrides auth file and defaults
        if( a_config != nullptr )
        {
            f_address = a_config->get_value( "broker", f_address );
            f_port = a_config->get_value( "broker-port", f_port );
            f_requests_exchange = a_config->get_value( "requests-exchange", f_requests_exchange );
            f_alerts_exchange = a_config->get_value( "alerts-exchange", f_alerts_exchange );
            f_make_connection = a_config->get_value( "make-connection", f_make_connection );
        }

        // parameters override config file, auth file, and defaults
        if( ! a_broker_address.empty() ) f_address = a_broker_address;
        if( a_port != 0 ) f_port = a_port;
    }

    //TODO having this constructor just because bools are 2-state and i can't tell default value from provided value == default
    core::core( const bool a_make_connection, const scarab::param_node* a_config ) :
            core::core(a_config)
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
            f_make_connection( a_orig.f_make_connection )
    {}

    core::core( core&& a_orig ) :
            f_address( std::move( a_orig.f_address ) ),
            f_port( a_orig.f_port ),
            f_username( std::move( a_orig.f_username ) ),
            f_password( std::move( a_orig.f_password ) ),
            f_requests_exchange( std::move( a_orig.f_requests_exchange ) ),
            f_alerts_exchange( std::move( a_orig.f_alerts_exchange) ),
            f_make_connection( std::move( a_orig.f_make_connection ) )
    {
        a_orig.f_port = 0;
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
        f_make_connection = std::move( a_orig.f_make_connection );
        return *this;
    }

    rr_pkg_ptr core::send( request_ptr_t a_request ) const
    {
        if ( ! f_make_connection )
        {
            LWARN( dlog, "send called but make_connection is false, returning nullptr" );
            return nullptr;
        }
        LDEBUG( dlog, "Sending request with routing key <" << a_request->routing_key() << ">" );
        rr_pkg_ptr t_receive_reply = std::make_shared< receive_reply_pkg >();
        t_receive_reply->f_channel = send_withreply( std::static_pointer_cast< message >( a_request ), t_receive_reply->f_consumer_tag, f_requests_exchange );
        t_receive_reply->f_successful_send = t_receive_reply->f_channel.get() != nullptr;
        return t_receive_reply;
    }

    bool core::send( reply_ptr_t a_reply ) const
    {
        if ( ! f_make_connection )
        {
            LWARN( dlog, "send called but make_connection is false, returning nullptr" );
            return false;
        }
        LDEBUG( dlog, "Sending reply with routing key <" << a_reply->routing_key() << ">" );
        return send_noreply( std::static_pointer_cast< message >( a_reply ), f_requests_exchange );
    }

    bool core::send( alert_ptr_t a_alert ) const
    {
        if ( ! f_make_connection )
        {
            LWARN( dlog, "send called but make_connection is false, returning nullptr" );
            return false;
        }
        LDEBUG( dlog, "Sending alert with routing key <" << a_alert->routing_key() << ">" );
        return send_noreply( std::static_pointer_cast< message >( a_alert ), f_alerts_exchange );
    }

    reply_ptr_t core::wait_for_reply( const rr_pkg_ptr a_receive_reply, int a_timeout_ms )
    {
        bool t_temp;
        return wait_for_reply( a_receive_reply, t_temp, a_timeout_ms );
    }

    reply_ptr_t core::wait_for_reply( const rr_pkg_ptr a_receive_reply, bool& a_chan_valid, int a_timeout_ms )
    {
        if ( ! a_receive_reply->f_channel )
        {
            //throw dripline_error() << "cannot wait for reply with make_connection is false";
            return reply_ptr_t();
        }
        LDEBUG( dlog, "Waiting for a reply" );

        amqp_envelope_ptr t_envelope;
        a_chan_valid = listen_for_message( t_envelope, a_receive_reply->f_channel, a_receive_reply->f_consumer_tag, a_timeout_ms );
        a_receive_reply->f_channel.reset();

        try
        {
            message_ptr_t t_message = message::process_envelope( t_envelope );

            if( t_message->is_reply() )
            {
                return std::static_pointer_cast< msg_reply >( t_message );
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

    amqp_channel_ptr core::send_withreply( message_ptr_t a_message, std::string& a_reply_consumer_tag, const std::string& a_exchange ) const
    {
        if ( ! f_make_connection )
        {
            throw dripline_error() << "cannot send reply with make_connection is false";
        }
        amqp_channel_ptr t_channel = open_channel();
        if( ! t_channel )
        {
            LERROR( dlog, "Unable to open channel to send a message to <" << a_message->routing_key() << "> using broker <" << f_address << ":" << f_port << ">" );
            return amqp_channel_ptr();
        }

        if( ! setup_exchange( t_channel, a_exchange ) )
        {
            LERROR( dlog, "Unable to setup the exchange <" << a_exchange << ">" );
            return amqp_channel_ptr();
        }

        // create the reply-to queue, and bind the queue to the routing key over the given exchange
        std::string t_reply_to = t_channel->DeclareQueue( "" );
        t_channel->BindQueue( t_reply_to, a_exchange, t_reply_to );
        a_message->reply_to() = t_reply_to;

        // begin consuming on the reply-to queue
        a_reply_consumer_tag = t_channel->BasicConsume( t_reply_to );
        LDEBUG( dlog, "Reply-to for request: " << t_reply_to );
        LDEBUG( dlog, "Consumer tag for reply: " << a_reply_consumer_tag );

        // convert the dripline::message object to an AMQP message
        amqp_message_ptr t_amqp_message = a_message->create_amqp_message();

        try
        {
            LDEBUG( dlog, "Sending message to <" << a_message->routing_key() << ">" );
            t_channel->BasicPublish( a_exchange, a_message->routing_key(), t_amqp_message, true, false );
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

    bool core::send_noreply( message_ptr_t a_message, const std::string& a_exchange ) const
    {
        if( ! f_make_connection )
        {
            LDEBUG( dlog, "does not make amqp connection, not sending payload:" );
            LDEBUG( dlog, a_message->get_payload() );
            throw dripline_error() << "cannot send message with make_connection is false";
            return true;
        }
        amqp_channel_ptr t_channel = open_channel();
        if( ! t_channel )
        {
            LERROR( dlog, "Unable to open channel to send a message to <" << a_message->routing_key() << "> using broker <" << f_address << ":" << f_port << ">" );
            return false;
        }

        if( ! setup_exchange( t_channel, a_exchange ) )
        {
            LERROR( dlog, "Unable to setup the exchange <" << a_exchange << ">" );
            return false;
        }

        amqp_message_ptr t_amqp_message = a_message->create_amqp_message();

        try
        {
            t_channel->BasicPublish( a_exchange, a_message->routing_key(), t_amqp_message, true, false );
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

    amqp_channel_ptr core::open_channel() const
    {
        if ( ! f_make_connection )
        {
            throw dripline_error() << "Should not call open_channel when f_make_connection is false";
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

    bool core::listen_for_message( amqp_envelope_ptr& a_envelope, amqp_channel_ptr a_channel, const std::string& a_consumer_tag, int a_timeout_ms )
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



} /* namespace dripline */

