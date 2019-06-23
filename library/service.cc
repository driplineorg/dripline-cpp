/*
 * service.cc
 *
 *  Created on: Jan 5, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "service.hh"

#include "authentication.hh"
#include "logger.hh"

using scarab::authentication;
using scarab::param_node;
using scarab::param_value;
using scarab::param_ptr_t;

using std::string;
using std::set;

namespace dripline
{
    LOGGER( dlog, "service" );

    service::service( const scarab::param_node& a_config, const string& a_queue_name,  const std::string& a_broker_address, unsigned a_port, const std::string& a_auth_file, const bool a_make_connection ) :
            core( a_config, a_broker_address, a_port, a_auth_file, a_make_connection ),
            // logic for setting the name:
            //   a_queue_name if provided
            //   otherwise a_config["queue"] if it exists
            //   otherwise "dlcpp_service"
            endpoint( a_queue_name.empty() ? a_config.get_value( "queue", "dlcpp_service" ) : a_queue_name, *this ),
            cancelable(),
            f_channel(),
            f_consumer_tag(),
            f_children(),
            f_broadcast_key( "broadcast" ),
            f_listen_timeout_ms( 500 )
    {
        // get values from the config
        f_listen_timeout_ms = a_config.get_value( "listen-timeout-ms", f_listen_timeout_ms );

        // override if specified as a separate argument
        if( ! a_queue_name.empty() ) f_name = a_queue_name;
    }

    service::service( const bool a_make_connection, const scarab::param_node& a_config ) :
            core( a_make_connection, a_config ),
            endpoint( "", *this ),
            cancelable(),
            f_channel(),
            f_consumer_tag(),
            f_children(),
            f_broadcast_key(),
            f_listen_timeout_ms( 500 )
    {
    }

    service::~service()
    {
    }

    rr_pkg_ptr service::send( request_ptr_t a_request ) const
    {
        a_request->set_sender_service_name( f_name );
        return core::send( a_request );
    }

    bool service::send( reply_ptr_t a_reply ) const
    {
        a_reply->set_sender_service_name( f_name );
        return core::send( a_reply );
    }

    bool service::send( alert_ptr_t a_alert ) const
    {
        a_alert->set_sender_service_name( f_name );
        return core::send( a_alert );
    }

    bool service::start()
    {
        if( ! f_make_connection )
        {
            LWARN( dlog, "Should not start service when make_connection is disabled" );
            return true;
        }
        if( f_name.empty() )
        {
            LERROR( dlog, "Service requires a queue name to be started" );
            return false;
        }

        LINFO( dlog, "Connecting to <" << f_address << ":" << f_port << ">" );

        f_channel = open_channel();
        if( ! f_channel ) return false;

        if( ! setup_exchange( f_channel, f_requests_exchange ) ) return false;
        if( ! setup_exchange( f_channel, f_alerts_exchange ) ) return false;

        if( ! setup_queue( f_channel, f_name ) ) return false;

        if( ! bind_keys() ) return false;

        if( ! start_consuming() ) return false;

        f_canceled.store( false );

        return true;
    }

    bool service::listen()
    {
        LINFO( dlog, "Listening for incoming messages on <" << f_name << ">" );

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
                // we end up here every time the listen times out with no message received
                continue;
            }

            try
            {
                message_ptr_t t_message = message::process_envelope( t_envelope );

                std::string t_first_token( t_message->routing_key() );
                t_first_token = t_first_token.substr( 0, t_first_token.find_first_of('.') );
                LDEBUG( dlog, "First token in routing key: <" << t_first_token << ">" );

                endpoint_ptr_t t_target;
                if( t_first_token == f_name || t_first_token == f_broadcast_key )
                {
                    do_on_message( this, t_message );
                }
                else
                {
                    auto t_endpoint_itr = f_children.find( t_first_token );
                    if( t_endpoint_itr == f_children.end() )
                    {
                        LERROR( dlog, "Did not find child endpoint called <" << t_first_token << ">" );
                        throw dripline_error() << "Did not find child endpoint <" << t_first_token << ">";
                    }

                    do_on_message( t_endpoint_itr->second, t_message );
                }
            }
            catch( dripline_error& e )
            {
                LERROR( dlog, "Dripline exception caught while handling message: " << e.what() );
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
        LINFO( dlog, "Stopping service on <" << f_name << ">" );

        if( ! stop_consuming() ) return false;

        if( ! remove_queue() ) return false;

        return true;
    }


    bool service::bind_keys()
    {
#ifdef DL_OFFLINE
        return false;
#endif

        try
        {
            for( child_map_t::const_iterator t_child_it = f_children.begin();
                    t_child_it != f_children.end();
                    ++t_child_it )
            {
                LDEBUG( dlog, "Binding key <" << t_child_it->first << ".#> to queue " << f_name );
                f_channel->BindQueue( f_name, f_requests_exchange, t_child_it->first + ".#" );
            }
            LDEBUG( dlog, "Binding key <" << f_name << ".#> to queue " << f_name );
            f_channel->BindQueue( f_name, f_requests_exchange, f_name + ".#" );
            LDEBUG( dlog, "Binding key <" << f_broadcast_key << ".#> to queue " << f_name );
            f_channel->BindQueue( f_name, f_requests_exchange, f_broadcast_key + ".#" );
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
#ifdef DL_OFFLINE
        return false;
#endif

        try
        {
            LDEBUG( dlog, "Starting to consume messages" );
            // second bool is setting no_ack to false
            f_consumer_tag = f_channel->BasicConsume( f_name, "", true, false );
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
#ifdef DL_OFFLINE
        return false;
#endif

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
#ifdef DL_OFFLINE
        return false;
#endif
        if ( ! f_make_connection )
        {
            LDEBUG( dlog, "no queue to remove because make_connection is false" );
            return true;
        }
        try
        {
            LDEBUG( dlog, "Deleting queue <" << f_name << ">" );
            f_channel->DeleteQueue( f_name, false );
            f_name.clear();
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
