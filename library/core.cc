/*
 * core.cc
 *
 *  Created on: Jun 27, 2017
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "core.hh"

#include "dripline_exceptions.hh"
#include "message.hh"

#include "authentication.hh"
#include "exponential_backoff.hh"
#include "logger.hh"
#include "param_codec.hh"
#include "signal_handler.hh"

#include <array>


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

    core::core( const scarab::param_node& a_config, const scarab::authentication& a_auth, const bool a_make_connection ) :
            f_address(),
            f_port(),
            f_username(),
            f_password(),
            f_requests_exchange(),
            f_alerts_exchange(),
            f_heartbeat_routing_key(),
            f_max_payload_size(),
            f_make_connection( a_make_connection ),
            f_max_connection_attempts()
    {
        // Get the default values, and merge in the supplied a_config
        // a_config's default value is also dripline_config, but the user can supply an arbitrary node.
        // So we need to assume no configuration values are supplied and we start again from dripline_config, then merge in a_config.
        dripline_config t_config;
        t_config.merge( a_config );
        LDEBUG( dlog, "Received config:\n" << a_config );
        LDEBUG( dlog, "Dripline core being configured with:\n" << t_config );

/* DO WE WANT TO USE ALTERNATIVE AUTH GROUPS?
        std::array< std::string > t_potential_groups{"dripline", "amqp", "rabbitmq"};
        std::string t_auth_group;
        for( const auto& i_gr : t_potential_goups )
        {
            if( a_auth.has( i_gr ) )
            {
                t_auth_group = i_gr;
                break;
            }
        }
        LDEBUG( dlog, "Using auth group <" << t_auth_group << ">" );

        f_username = t_auth.get( t_auth_group, "username", f_username );
        f_password = t_auth.get( t_auth_group, "password", f_password );
*/
        // Replace local parameters with values from the config
        f_address = t_config["broker"]().as_string(); //.get_value("broker", "localhost");
        f_port = t_config["broker_port"]().as_uint(); //.get_value("broker_port", 5672);
        f_requests_exchange = t_config["requests_exchange"]().as_string(); //.get_value("requests_exchange", "requests");
        f_alerts_exchange = t_config["alerts_exchange"]().as_string(); //.get_value("alerts_exchange", "alerts");
        f_heartbeat_routing_key = t_config["heartbeat_routing_key"]().as_string(); //.get_value("heartbeat_routing_key", "heartbeat");
        f_max_payload_size = t_config["max_payload_size"]().as_uint(); //.get_value("max_payload_size", DL_MAX_PAYLOAD_SIZE);
        f_max_connection_attempts = t_config["max_connection_attempts"]().as_uint(); //.get_value("max_connection_attempts", 10);

        f_username = a_auth.get("dripline", "username", "guest");
        f_password = a_auth.get("dripline", "password", "guest");

        // additional return codes
        if( t_config.has( "return_codes" ) )
        {
            // define a function for extracting return codes from a param_array so that we can use it in a couple places
            auto t_extract_codes = [](const scarab::param_array& a_codes)
            {
                LDEBUG( dlog, "Adding return codes:\n" << a_codes );
                for( auto t_code_it = a_codes.begin(); t_code_it != a_codes.end(); ++t_code_it )
                {
                    try
                    {
                        const scarab::param_node& t_a_node = t_code_it->as_node();
                        if( check_and_add_return_code( t_a_node["value"]().as_uint(), t_a_node["name"]().as_string(), t_a_node["description"]().as_string() ) )
                        {
                            LDEBUG( dlog, "Added return code <" << t_a_node["name"]().as_string() << " (" << t_a_node["value"]().as_uint() << ")>: " << t_a_node["description"]().as_string() );
                        }
                    }
                    catch( const scarab::error& e )
                    {
                        throw dripline_error() << "Invalid configuration for a return code:\n" << *t_code_it << '\n' << e.what();
                    }
                    catch( const std::out_of_range& e )
                    {
                        throw dripline_error() << "Missing configuration parameter for a return code:\n" << *t_code_it;
                    }
                }
                return;
            };

            if( t_config["return_codes"].is_value() && t_config["return_codes"]().is_string() )
            {
                // then it's a filename; load YAML
                std::string t_filename( t_config["return_codes"]().as_string() );
                scarab::param_translator t_translator;
                scarab::param_ptr_t t_ret_codes = t_translator.read_file( t_filename );
                if( ! t_ret_codes || ! t_ret_codes->is_array() )
                {
                    throw dripline_error() << "Could not find or open return-code config file, or the config does not contain an array: " << t_filename;
                }
                t_extract_codes( t_ret_codes->as_array() );
            }
            else if( t_config["return_codes"].is_array() )
            {
                // then individual codes are specified
                t_extract_codes( t_config["return_codes"].as_array() );
            }
            else
            {
                throw dripline_error() << "Return code configuration is invalid:\n" << t_config["return_codes"];
            }
        }
    }

    sent_msg_pkg_ptr core::send( request_ptr_t a_request, amqp_channel_ptr a_channel ) const
    {
        LDEBUG( dlog, "Sending request with routing key <" << a_request->routing_key() << ">" );
        return do_send( std::static_pointer_cast< message >( a_request ), f_requests_exchange, true, a_channel );
    }

    sent_msg_pkg_ptr core::send( reply_ptr_t a_reply, amqp_channel_ptr a_channel ) const
    {
        LDEBUG( dlog, "Sending reply with routing key <" << a_reply->routing_key() << ">" );
        return do_send( std::static_pointer_cast< message >( a_reply ), f_requests_exchange, false, a_channel );
    }

    sent_msg_pkg_ptr core::send( alert_ptr_t a_alert, amqp_channel_ptr a_channel ) const
    {
        LDEBUG( dlog, "Sending alert with routing key <" << a_alert->routing_key() << ">" );
        return do_send( std::static_pointer_cast< message >( a_alert ), f_alerts_exchange, false, a_channel );
    }

    sent_msg_pkg_ptr core::do_send( message_ptr_t a_message, const std::string& a_exchange, bool a_expect_reply, amqp_channel_ptr a_channel ) const
    {
        // throws connection_error if it could not connect with the broker
        // throws dripline_error if there's a problem with the exchange or creating the AMQP message object(s)
        // returns the receive_reply package if the message was completely or partially sent
        // the f_successful_send flag will be set accordingly: true if completely sent; false if partially sent
        // if there was an error sending the message, that will be returned in f_send_error_message, which will be empty otherwise

        // lambda to create a string with the basic information about the send attempt
        auto t_diagnostic_string_maker = [a_message, this]() -> std::string {
            return std::string("Broker: ") + f_address +"\nPort: " + std::to_string(f_port) + "\nRouting Key: " + a_message->routing_key();
        };

        if ( ! f_make_connection || core::s_offline )
        {
            throw a_message;
            //throw dripline_error() << "cannot send reply when make_connection is false";
        }

        amqp_channel_ptr t_channel = a_channel ? a_channel : open_channel();
        if( ! t_channel )
        {
            throw connection_error() << "Unable to open channel to send message\n" << t_diagnostic_string_maker();
        }

        if( ! setup_exchange( t_channel, a_exchange ) )
        {
            throw dripline_error() << "Unable to setup the exchange <" << a_exchange << "> to send message\n" << t_diagnostic_string_maker();
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
            throw dripline_error() << "Unable to convert the dripline::message object to AMQP message(s) to be sent\n" << t_diagnostic_string_maker();
        }

        try
        {
            LDEBUG( dlog, "Sending message to <" << a_message->routing_key() << ">" );
            for( amqp_message_ptr& t_amqp_message : t_amqp_messages )
            {
                // send the message
                // the first boolean argument is whether it's mandatory that the message be delivered to a queue.
                // this is only the case for requests, where we expect something to be listening.
                t_channel->BasicPublish( a_exchange, a_message->routing_key(), t_amqp_message, a_message->is_request(), false );
            }
            LDEBUG( dlog, "Message sent in " << t_amqp_messages.size() << " chunks" );
            t_receive_reply->f_successful_send = true;
            t_receive_reply->f_send_error_message.clear();
        }
        catch( AmqpClient::ConnectionClosedException& e )
        {
            LERROR( dlog, "Unable to send message because the connection is closed: " << e.what() );
            throw connection_error() << "Unable to send message because the connection is closed: " << e.what() << '\n' << t_diagnostic_string_maker();
        }
        catch( AmqpClient::AmqpLibraryException& e )
        {
            LERROR( dlog, "AMQP error while sending message: " << e.what() );
            t_receive_reply->f_successful_send = false;
            t_receive_reply->f_send_error_message = std::string("AMQP error while sending message: ") + std::string(e.what()) + '\n' + t_diagnostic_string_maker();
        }
        catch( AmqpClient::MessageReturnedException& e )
        {
            LERROR( dlog, "Message was returned: " << e.what() );
            t_receive_reply->f_successful_send = false;
            t_receive_reply->f_send_error_message = std::string("Message was returned: ") + std::string(e.what()) + '\n' + t_diagnostic_string_maker();
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Error while sending message: " << e.what() );
            t_receive_reply->f_successful_send = false;
            t_receive_reply->f_send_error_message = std::string("Error while sending message: ") + std::string(e.what()) + '\n' + t_diagnostic_string_maker();
        }

        return t_receive_reply;
    }

    amqp_channel_ptr core::open_channel() const
    {
        // Exceptions that can be encountered while opening a channel
        //   SimpleAmqpClient::Channel::Open(opts)
        //       std::runtime_error -- options are invalid; auth not specified
        //       std::logic_error -- unhandled auth type
        //       std::bad_alloc -- connection is null
        //       amqp_exception -- unsure of what would cause this
        //       amqp_lib_exception -- unable to make connection to the broker; maybe other things

        if ( ! f_make_connection || core::s_offline )
        {
            return amqp_channel_ptr();
            //throw dripline_error() << "Should not call open_channel when offline";
        }

        amqp_channel_ptr t_ret_ptr = amqp_channel_ptr();

        auto t_open_conn_fcn = [&]()->bool
        {
            try
            {
                LINFO( dlog, "Opening AMQP connection and creating channel to " << f_address << ":" << f_port );
                LDEBUG( dlog, "Using broker authentication: " << f_username << ":" << f_password );
                struct AmqpClient::Channel::OpenOpts opts;
                opts.host = f_address;
                opts.port = f_port;
                opts.auth = AmqpClient::Channel::OpenOpts::BasicAuth(f_username, f_password);
                t_ret_ptr = AmqpClient::Channel::Open( opts );
                return true;
            }
            catch( amqp_exception& e )
            {
                if( e.is_soft_error() ) 
                {
                    LWARN( dlog, "Recoverable AMQP exception caught while opening channel: (" << e.reply_code() << ") " << e.reply_text() );
                    return false;
                }
                // otherwise error is non-recoverable
                throw;
            }
            catch( amqp_lib_exception& e )
            {
                LERROR( dlog, "AMQP Library Exception caught while creating channel: (" << e.ErrorCode() << ") " << e.what() );
                if( e.ErrorCode() == -9 )
                {
                    LERROR( dlog, "This error means the client could not connect to the broker.\n\t" <<
                            "Check that you have the address and port correct, and that the broker is running.")
                }
                return false;
            }
            // std::exceptions are non-recoverable, so don't catch them
        };

        scarab::exponential_backoff<> t_open_conn_backoff( t_open_conn_fcn, f_max_connection_attempts );
        auto t_exp_cancel_wrap = wrap_cancelable( t_open_conn_backoff );
        scarab::signal_handler::add_cancelable( t_exp_cancel_wrap );

        try
        {
            LDEBUG( dlog, "Attempting to open channel; will make up to " << f_max_connection_attempts << " attempts" );
            t_open_conn_backoff.go();
            // either succeeded or failed after multiple attempts
        }
        catch( amqp_exception& e )
        {
            LERROR( dlog, "Unrecoverable AMQP exception caught while opening channel: (" << e.reply_code() << ") " << e.reply_text() );
        }
        catch(const std::exception& e)
        {
            // unrecoverable error causing a std::exception
            LERROR( dlog, "Standard exception caught while creating channel: " << e.what() );
        }
        
        return t_ret_ptr;
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

    bool core::bind_key( amqp_channel_ptr a_channel, const std::string& a_exchange, const std::string& a_queue_name, const std::string& a_routing_key )
    {
        if( s_offline || ! a_channel )
        {
            return false;
        }

        try
        {
            LDEBUG( dlog, "Binding key <" << a_routing_key << "> to queue <" << a_queue_name << "> over exchange <" << a_exchange << ">" );
            a_channel->BindQueue( a_queue_name, a_exchange, a_routing_key );

            return true;
        }
        catch( amqp_exception& e )
        {
            LERROR( dlog, "AMQP exception caught while declaring binding key <" << a_routing_key << ">: (" << e.reply_code() << ") " << e.reply_text() );
            return false;
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "AMQP library exception caught while binding key <" << a_routing_key << ">: (" << e.ErrorCode() << ") " << e.what() );
            return false;
        }
    }

    std::string core::start_consuming( amqp_channel_ptr a_channel, const std::string& a_queue_name )
    {
        if( s_offline || ! a_channel )
        {
            return std::string();
        }

        try
        {
            LDEBUG( dlog, "Starting to consume messages on queue <" << a_queue_name << ">" );
            // second bool is setting no_ack to false
            return a_channel->BasicConsume( a_queue_name, "", true, false );
        }
        catch( amqp_exception& e )
        {
            LERROR( dlog, "AMQP exception caught while starting consuming messages on <" << a_queue_name << ">: (" << e.reply_code() << ") " << e.reply_text() );
            return std::string();
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "AMQP library exception caught while starting consuming messages on <" << a_queue_name << ">: (" << e.ErrorCode() << ") " << e.what() );
            return std::string();
        }
    }

    bool core::stop_consuming( amqp_channel_ptr a_channel, std::string& a_consumer_tag )
    {
        if( s_offline || ! a_channel )
        {
            return false;
        }

        try
        {
            LDEBUG( dlog, "Stopping consuming messages for consumer <" << a_consumer_tag << ">" );
            a_channel->BasicCancel( a_consumer_tag );
            a_consumer_tag.clear();
            return true;
        }
        catch( amqp_exception& e )
        {
            LERROR( dlog, "AMQP exception caught while stopping consuming messages on <" << a_consumer_tag << ">: (" << e.reply_code() << ") " << e.reply_text() );
            return false;
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "AMQP library exception caught while stopping consuming messages on <" << a_consumer_tag << ">: (" << e.ErrorCode() << ") " << e.what() );
            return false;
        }
        catch( AmqpClient::ConsumerTagNotFoundException& e )
        {
            LERROR( dlog, "Fatal AMQP exception encountered while stopping consuming messages on <" << a_consumer_tag << ">: " << e.what() );
            return false;
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Standard exception caught while stopping consuming messages on <" << a_consumer_tag << ">: " << e.what() );
            return false;
        }
    }

    bool core::remove_queue( amqp_channel_ptr a_channel, const std::string& a_queue_name )
    {
        if( s_offline || ! a_channel )
        {
            return false;
        }

        try
        {
            LDEBUG( dlog, "Deleting queue <" << a_queue_name << ">" );
            a_channel->DeleteQueue( a_queue_name, false );
            return true;
        }
        catch( AmqpClient::ConnectionClosedException& e )
        {
            LERROR( dlog, "Fatal AMQP exception encountered removing queue <" << a_queue_name << ">: " << e.what() );
            return false;
        }
        catch( amqp_lib_exception& e )
        {
            LERROR( dlog, "AMQP library exception caught while removing queue <" << a_queue_name << ">: (" << e.ErrorCode() << ") " << e.what() );
            return false;
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Standard exception caught while removing queue <" << a_queue_name << ">: " << e.what() );
            return false;
        }
    }

    void core::listen_for_message( amqp_envelope_ptr& a_envelope, core::post_listen_status& a_status, amqp_channel_ptr a_channel, const std::string& a_consumer_tag, int a_timeout_ms, bool a_do_ack )
    {
        if( s_offline || ! a_channel )
        {
            a_status = core::post_listen_status::unknown;
            return;
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
                if( a_envelope )
                {
                    if( a_do_ack )  a_channel->BasicAck( a_envelope );
                    a_status = post_listen_status::message_received;
                }
                else
                {
                    a_status = post_listen_status::timeout;
                }
                return;
            }
            catch( AmqpClient::ConnectionClosedException& e )
            {
                LERROR( dlog, "Fatal AMQP exception encountered: " << e.what() );
                a_status = post_listen_status::hard_error;
                return;
            }
            catch( AmqpClient::ConsumerCancelledException& e )
            {
                LERROR( dlog, "Fatal AMQP exception encountered: " << e.what() );
                a_status = post_listen_status::hard_error;
                return;
            }
            catch( AmqpClient::AmqpException& e )
            {
                if( e.is_soft_error() )
                {
                    LWARN( dlog, "Non-fatal AMQP exception encountered: " << e.reply_text() );
                    a_status = post_listen_status::soft_error;
                    return;
                }
                LERROR( dlog, "Fatal AMQP exception encountered: " << e.reply_text() );
                a_status = post_listen_status::hard_error;
                return;
            }
            catch( std::exception& e )
            {
                LERROR( dlog, "Standard exception caught: " << e.what() );
                a_status = post_listen_status::hard_error;
                return;
            }
            catch(...)
            {
                LERROR( dlog, "Unknown exception caught" );
                a_status = post_listen_status::hard_error;
                return;
            }
        }
    }

} /* namespace dripline */

