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

#include "rmqa_consumer.h"
#include "rmqa_rabbitcontext.h"
#include "rmqa_vhost.h"
#include "rmqa_producer.h"
#include "rmqp_messageguard.h"
#include "rmqt_consumerconfig.h"
#include "rmqt_queue.h"
#include "rmqt_simpleendpoint.h"
#include "rmqt_plaincredentials.h"
#include "rmqt_properties.h"

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
        if( f_reply_consumer )
        {
            LDEBUG( dlog, "Canceling reply consumer" );
            auto t_result = f_reply_consumer->cancelAndDrain();
            if( ! t_result )
            {
                LWARN( dlog, "Error while canceling reply consumer: " << t_result.error() );
            }
        }
    }

    bool core::s_offline = false;

    core::core( const scarab::param_node& a_config, const scarab::authentication& a_auth, const bool a_make_connection ) :
            f_address(),
            f_port(),
            f_username(),
            f_password(),
            f_heartbeat_routing_key(),
            f_max_payload_size(),
            f_make_connection(),
            f_max_connection_attempts(),
            f_rabbit_context(),
            f_vhost(),
            f_connection_mutex( std::make_shared< std::mutex >() )
    {
        // Get the default values, and merge in the supplied a_config
        // a_config's default value is also dripline_config, but the user can supply an arbitrary node.
        // So we need to assume no configuration values are supplied and we start again from dripline_config, then merge in a_config.
        dripline_config t_config;
        t_config.merge( a_config );
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
        f_address = t_config["broker"]().as_string();
        f_port = t_config["broker_port"]().as_uint();
        f_requests_ex.f_name = t_config["requests_exchange"]().as_string();
        f_alerts_ex.f_name = t_config["alerts_exchange"]().as_string();
        f_heartbeat_routing_key = t_config["heartbeat_routing_key"]().as_string();
        f_make_connection = t_config.get_value( "make_connection", a_make_connection );
        f_max_payload_size = t_config["max_payload_size"]().as_uint();
        f_max_connection_attempts = t_config["max_connection_attempts"]().as_uint();

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

    sent_msg_pkg_ptr core::send( request_ptr_t a_request ) const
    {
        LDEBUG( dlog, "Sending request with routing key <" << a_request->routing_key() << ">" );
        if ( ! f_make_connection || core::s_offline )
        {
            throw a_request;
        }
        return do_send( std::static_pointer_cast< message >( a_request ), f_requests_ex, true );
    }

    sent_msg_pkg_ptr core::send( reply_ptr_t a_reply ) const
    {
        LDEBUG( dlog, "Sending reply with routing key <" << a_reply->routing_key() << ">" );
        if ( ! f_make_connection || core::s_offline )
        {
            throw a_reply;
        }
        return do_send( std::static_pointer_cast< message >( a_reply ), f_requests_ex, false );
    }

    sent_msg_pkg_ptr core::send( alert_ptr_t a_alert ) const
    {
        LDEBUG( dlog, "Sending alert with routing key <" << a_alert->routing_key() << ">" );
        if ( ! f_make_connection || core::s_offline )
        {
            throw a_alert;
        }
        return do_send( std::static_pointer_cast< message >( a_alert ), f_alerts_ex, false );
    }

    sent_msg_pkg_ptr core::do_send( message_ptr_t a_message, exchange_store& a_exchange, bool a_expect_reply ) const
    {
        // throws connection_error if it could not connect with the broker
        // throws dripline_error if there's a problem creating the AMQP message object(s)
        // returns the sent_msg_pkg; f_successful_send indicates whether the send succeeded

        auto t_diagnostic_string_maker = [a_message, this]() -> std::string {
            return std::string("Broker: ") + f_address + "\nPort: " + std::to_string(f_port) + "\nRouting Key: " + a_message->routing_key();
        };

        open_connection();

        if( ! f_vhost )
        {
            throw connection_error() << "Not connected to broker\n" << t_diagnostic_string_maker();
        }

        sent_msg_pkg_ptr t_receive_reply = std::make_shared< sent_msg_pkg >();

        try
        {
            if( a_expect_reply )
            {
                send_withreply( a_message, a_exchange, t_receive_reply );
            }
            else
            {
                if( ! send_noreply( a_message, a_exchange ) )
                {
                    t_receive_reply->f_successful_send = false;
                    t_receive_reply->f_send_error_message = "Error in send_noreply\n" + t_diagnostic_string_maker();
                    return t_receive_reply;
                }
            }
            LDEBUG( dlog, "Message sent to <" << a_message->routing_key() << ">" );
            t_receive_reply->f_successful_send = true;
            t_receive_reply->f_send_error_message.clear();
        }
        catch( connection_error& )
        {
            throw;
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Error while sending message: " << e.what() );
            t_receive_reply->f_successful_send = false;
            t_receive_reply->f_send_error_message = std::string("Error while sending message: ") + e.what() + '\n' + t_diagnostic_string_maker();
        }

        return t_receive_reply;
    }

    void core::open_connection() const
    {
        std::lock_guard< std::mutex > t_lock( *f_connection_mutex );

        if( f_vhost )
        {
            return; // already connected
        }
        if( ! f_make_connection || s_offline )
        {
            return;
        }

        LINFO( dlog, "Opening AMQP connection to " << f_address << ":" << f_port );
        LDEBUG( dlog, "Using broker authentication: " << f_username << ":" << f_password );

        using namespace BloombergLP;

        // Set any options we need in the RabbitContext
        rmqa::RabbitContextOptions t_options;
        using namespace bsls::TimeIntervalLiterals;
        t_options.setConnectionErrorThreshold( 10_s ); // TODO: make this appropriately configurable via dripline_config

        f_rabbit_context = bsl::make_shared< rmqa::RabbitContext >( t_options );

        auto t_endpoint = bsl::make_shared< rmqt::SimpleEndpoint >( f_address, "/", (bsl::uint16_t)f_port );
        auto t_credentials = bsl::make_shared< rmqt::PlainCredentials >( f_username, f_password );
        f_vhost = f_rabbit_context->createVHostConnection( "dripline", t_endpoint, t_credentials );

        if( ! f_vhost )
        {
            f_rabbit_context.reset();
            throw connection_error() << "Unable to create vhost connection to " << f_address << ":" << f_port;
        }

        // Declare both exchanges
        f_requests_ex.create_exchange();
        f_alerts_ex.create_exchange();
        //f_requests_ex.f_exchange = f_topology.addExchange( bsl::string(f_requests_ex.f_name), rmqt::ExchangeType::TOPIC, rmqt::AutoDelete::OFF, rmqt::Durable::ON, rmqt::Internal::NO );
        //f_alerts_ex.f_exchange   = f_topology.addExchange( bsl::string(f_alerts_ex.f_name),   rmqt::ExchangeType::TOPIC, rmqt::AutoDelete::OFF, rmqt::Durable::ON, rmqt::Internal::NO );

        // Create requests producer
        {
            auto t_result = f_vhost->createProducer( f_requests_ex.f_topology, f_requests_ex.f_exchange, 10 );
            if( ! t_result )
            {
                f_vhost.reset();
                f_rabbit_context.reset();
                throw connection_error() << "Unable to create requests producer: " << t_result.error();
            }
            f_requests_ex.f_producer = t_result.value();
        }

        // Create alerts producer
        {
            auto t_result = f_vhost->createProducer( f_alerts_ex.f_topology, f_alerts_ex.f_exchange, 10 );
            if( ! t_result )
            {
                f_requests_ex.f_producer.reset();
                f_vhost.reset();
                f_rabbit_context.reset();
                throw connection_error() << "Unable to create alerts producer: " << t_result.error();
            }
            f_alerts_ex.f_producer = t_result.value();
        }

        LINFO( dlog, "AMQP connection established" );
    }

    BloombergLP::rmqt::QueueHandle core::add_requests_queue( const std::string& a_queue_name, 
                bool a_auto_delete, bool a_durable, 
                const scarab::param_node& a_field_table )
    {
        return f_requests_ex.add_queue( a_queue_name, a_auto_delete, a_durable, a_field_table );
    }

//    BloombergLP::rmqt::QueueHandle core::add_requests_ephemeral_queue( const std::string& a_queue_name )
//    {
//        return f_requests_ex.add_ephemeral_queue( f_topology, a_queue_name );
//    }

    void core::bind_requests_key( const std::string& a_queue_name, const std::string& a_routing_key, BloombergLP::rmqt::QueueHandle a_queue )
    {
        f_requests_ex.bind_key( a_queue_name, a_routing_key, a_queue );
    }

    BloombergLP::rmqt::QueueHandle core::add_alerts_queue( const std::string& a_queue_name, 
                bool a_auto_delete, bool a_durable, 
                const scarab::param_node& a_field_table )
    {
        return f_alerts_ex.add_queue( a_queue_name, a_auto_delete, a_durable, a_field_table );
    }

//    BloombergLP::rmqt::QueueHandle core::add_alerts_ephemeral_queue( const std::string& a_queue_name )
//    {
//        return f_alerts_ex.add_ephemeral_queue( f_topology, a_queue_name );
//    }

    void core::bind_alerts_key( const std::string& a_queue_name, const std::string& a_routing_key, BloombergLP::rmqt::QueueHandle a_queue )
    {
        f_alerts_ex.bind_key( a_queue_name, a_routing_key, a_queue );
    }

    //***************************
    // Exchange store definitions
    //***************************

    void core::exchange_store::create_exchange()
    {
        using namespace BloombergLP;
        f_exchange = f_topology.addExchange( bsl::string(f_name), rmqt::ExchangeType::TOPIC, rmqt::AutoDelete::OFF, rmqt::Durable::ON, rmqt::Internal::NO );
        return;
    }

    BloombergLP::rmqt::QueueHandle core::exchange_store::add_queue( const std::string& a_queue_name, 
                bool a_auto_delete, bool a_durable, 
                const scarab::param_node& a_field_table )
    {
        using namespace BloombergLP;
        LDEBUG( dlog, "Declaring durable queue <" << a_queue_name << "> on exchange <" << f_name << ">" );
        bsl::shared_ptr<rmqt::FieldTable> t_bsl_field_table = param_to_table(a_field_table).the< bsl::shared_ptr<rmqt::FieldTable> >();
        rmqt::QueueHandle t_queue = f_topology.addQueue( bsl::string(a_queue_name), 
                rmqt::AutoDelete::Value(int(a_auto_delete)), rmqt::Durable::Value(int(a_durable)), 
                *t_bsl_field_table );
        if( ! t_queue.lock() )
        {
            throw dripline_error() << "Queue could not be created. See log for error message.";
        }
        return t_queue;
    }

//    BloombergLP::rmqt::QueueHandle core::exchange_store::add_ephemeral_queue( BloombergLP::rmqa::Topology& a_topo, const std::string& a_queue_name )
//    {
//        using namespace BloombergLP;
//        LDEBUG( dlog, "Declaring ephemeral queue <" << a_queue_name << "> on exchange <" << f_name << ">" );
//        return a_topo.addQueue( bsl::string(a_queue_name), rmqt::AutoDelete::ON, rmqt::Durable::ON );
//    }

    void core::exchange_store::bind_key( const std::string& a_queue_name, const std::string& a_routing_key, BloombergLP::rmqt::QueueHandle a_queue )
    {
        LDEBUG( dlog, "Binding queue <" << a_queue_name << "> to exchange <" << f_name << "> with routing key <" << a_routing_key << ">" );
        f_topology.bind( f_exchange,
                         a_queue,
                         bsl::string(a_routing_key) );
    }

    //***************************
    //***************************

    void core::send_withreply( message_ptr_t a_message, exchange_store& a_exchange, sent_msg_pkg_ptr a_pkg ) const
    {
        using namespace BloombergLP;

        // Generate a unique name for the temporary reply queue
        std::string t_reply_to = string_from_uuid( generate_random_uuid() );
        a_message->reply_to() = t_reply_to;
        LDEBUG( dlog, "Reply-to for request: " << t_reply_to );

        // Build a local topology for the temporary reply queue only.
        // This topology is separate from f_topology so that the transient queue does not
        // pollute the persistent topology passed to message_dispatcher::start_listening().
        rmqa::Topology t_reply_topo;
        auto t_ex = t_reply_topo.addPassiveExchange( a_exchange.f_name );
        if( ! t_ex.lock() )
        {
            throw connection_error() << "Unable to use exchange <" << a_exchange.f_name << ">";
        }
        // Queue properties:
        //   Exclusive: OFF -- currently the only option given by the rmqcpp API
        //   Auto-delete: ON -- Queue will be deleted after the last consumer disconnects
        //   Durable: ON -- Queue will survive if broker is disrupted
        auto t_queue = t_reply_topo.addQueue( t_reply_to, rmqt::AutoDelete::ON, rmqt::Durable::ON );
        if( ! t_queue.lock() )
        {
            throw connection_error() << "Unable to create queue to receive reply";
        }
        t_reply_topo.bind( t_ex, t_queue, t_reply_to );

        // Create promise; the consumer callback will fulfil it when the complete reply is assembled
        a_pkg->f_reply_promise = std::make_shared< std::promise< reply_ptr_t > >();
        auto t_promise = a_pkg->f_reply_promise;

        // Inline accumulator struct for multi-chunk replies
        struct reply_pack
        {
            amqp_split_message_ptrs f_messages;
            unsigned f_chunks_received;
            std::string f_routing_key;
            std::mutex f_mutex;
            reply_pack() : f_messages(), f_chunks_received( 0 ), f_routing_key(), f_mutex() {}
        };
        auto t_pack = std::make_shared< reply_pack >();

        auto t_callback = [t_pack, t_promise]( rmqp::MessageGuard& guard )
        {
            auto t_amqp_message = bsl::make_shared< rmqt::Message >( guard.message() );
            const std::string t_msg_id( t_amqp_message->messageId() );
            auto t_parsed_id = message::parse_message_id( t_msg_id );
            unsigned t_chunk_idx = std::get< 1 >( t_parsed_id );
            unsigned t_total_chunks = std::get< 2 >( t_parsed_id );

            bool t_complete = false;
            {
                std::lock_guard< std::mutex > t_lock( t_pack->f_mutex );
                if( t_pack->f_messages.empty() )
                {
                    t_pack->f_messages.resize( t_total_chunks );
                    t_pack->f_routing_key = std::string( guard.envelope().routingKey() );
                }
                t_pack->f_messages[t_chunk_idx] = t_amqp_message;
                ++t_pack->f_chunks_received;
                t_complete = ( t_pack->f_chunks_received == t_pack->f_messages.size() );
            }

            guard.ack();

            if( t_complete )
            {
                try
                {
                    message_ptr_t t_message = message::process_message( t_pack->f_messages, t_pack->f_routing_key );
                    reply_ptr_t t_reply = std::dynamic_pointer_cast< msg_reply >( t_message );
                    if( t_reply )
                    {
                        t_promise->set_value( t_reply );
                    }
                    else
                    {
                        t_promise->set_exception( std::make_exception_ptr(
                            dripline_error() << "Expected reply but received a different message type" ) );
                    }
                }
                catch( ... )
                {
                    t_promise->set_exception( std::current_exception() );
                }
            }
        };

        // Create the reply consumer on the temporary queue
        rmqt::ConsumerConfig t_consumer_conf( "reply-" + t_reply_to, 1, 0, rmqt::Exclusive::ON );
        auto t_consumer_result = f_vhost->createConsumer( t_reply_topo, t_queue, t_callback, t_consumer_conf );
        if( ! t_consumer_result )
        {
            throw connection_error() << "Unable to create reply consumer: " << t_consumer_result.error();
        }
        a_pkg->f_reply_consumer = t_consumer_result.value();

        // Send message chunks via the requests producer
        amqp_split_message_ptrs t_amqp_messages = a_message->create_amqp_messages( f_max_payload_size );
        if( t_amqp_messages.empty() )
        {
            throw dripline_error() << "Unable to convert dripline::message to AMQP message(s)";
        }

        LDEBUG( dlog, "Sending request to <" << a_message->routing_key() << "> in " << t_amqp_messages.size() << " chunk(s)" );
        for( amqp_message_ptr& t_amqp_message : t_amqp_messages )
        {
            auto t_status = f_requests_ex.f_producer->send(
                *t_amqp_message,
                a_message->routing_key(),
                []( const rmqt::Message&, const bsl::string&, const rmqt::ConfirmResponse& ) {} );
            if( t_status != rmqp::Producer::SENDING )
            {
                throw dripline_error() << "Failed to enqueue request chunk for sending; status=" << t_status;
            }
        }
    }

    bool core::send_noreply( message_ptr_t a_message, exchange_store& a_exchange ) const
    {
        using namespace BloombergLP;

        bsl::shared_ptr< rmqa::Producer > t_producer = a_exchange.f_producer;
        if( ! t_producer )
        {
            LERROR( dlog, "No producer available for exchange <" << a_exchange.f_name << ">" );
            return false;
        }

        amqp_split_message_ptrs t_amqp_messages = a_message->create_amqp_messages( f_max_payload_size );
        if( t_amqp_messages.empty() )
        {
            LERROR( dlog, "Unable to convert dripline::message to AMQP message(s)" );
            return false;
        }

        LDEBUG( dlog, "Sending message to <" << a_message->routing_key() << "> in " << t_amqp_messages.size() << " chunk(s)" );
        bool t_all_sent = true;
        for( amqp_message_ptr& t_amqp_message : t_amqp_messages )
        {
            auto t_status = t_producer->send(
                *t_amqp_message,
                a_message->routing_key(),
                []( const rmqt::Message&, const bsl::string&, const rmqt::ConfirmResponse& ) {} );
            if( t_status != rmqp::Producer::SENDING )
            {
                LERROR( dlog, "Failed to enqueue message chunk for sending; status=" << t_status );
                t_all_sent = false;
            }
        }
        return t_all_sent;
    }

} /* namespace dripline */
