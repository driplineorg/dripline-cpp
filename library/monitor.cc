/*
 * monitor.cc
 *
 *  Created on: Jul 1, 2019
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "monitor.hh"

#include "dripline_exceptions.hh"
#include "uuid.hh"

#include "logger.hh"
#include "signal_handler.hh"

LOGGER( dlog, "monitor" );

namespace dripline
{

    monitor::monitor( const scarab::param_node& a_config ) :
            scarab::cancelable(),
            core( a_config.has( "dripline" ) ? a_config["dripline"].as_node() : scarab::param_node() ),
            listener_receiver(),
            f_status( status::nothing ),
            f_name( std::string("monitor_") + string_from_uuid(generate_random_uuid()) ),
            f_json_print( false ),
            f_pretty_print( false ),
            f_requests_keys(),
            f_alerts_keys()
    {
        // get requests keys
        if( a_config.has( "request-keys" ) && a_config["request-keys"].is_array() )
        {
            const scarab::param_array& t_req_keys = a_config["request-keys"].as_array();
            f_requests_keys.reserve( t_req_keys.size() );
            for( auto t_it = t_req_keys.begin(); t_it != t_req_keys.end(); ++t_it )
            {
                LPROG( dlog, "Monitor <" << f_name << "> will monitor key <" << (*t_it)().as_string() << "> on the requests exchange" );
                f_requests_keys.push_back( (*t_it)().as_string() );
            }
        }

        if( a_config.has( "request-key" ) && a_config["request-key"].is_value() )
        {
            LPROG( dlog, "Monitor <" << f_name << "> will monitor key <" << a_config["request-key"]().as_string() << "> on the requests exchange" );
            f_requests_keys.push_back( a_config["request-key"]().as_string() );
        }

        // get alerts keys
        if( a_config.has( "alert-keys" ) && a_config["alert-keys"].is_array() )
        {
            const scarab::param_array& t_req_keys = a_config["alert-keys"].as_array();
            f_requests_keys.reserve( t_req_keys.size() );
            for( auto t_it = t_req_keys.begin(); t_it != t_req_keys.end(); ++t_it )
            {
                LPROG( dlog, "Monitor <" << f_name << "> will monitor key <" << (*t_it)().as_string() << "> on the alerts exchange" );
                f_alerts_keys.push_back( (*t_it)().as_string() );
            }
        }

        if( a_config.has( "alert-key" ) && a_config["alert-key"].is_value() )
        {
            LPROG( dlog, "Monitor <" << f_name << "> will monitor key <" << a_config["alert-key"]().as_string() << "> on the alerts exchange" );
            f_alerts_keys.push_back( a_config["alert-key"]().as_string() );
        }
    }

    monitor::monitor( monitor&& a_orig ) :
            scarab::cancelable( std::move(a_orig) ),
            core( std::move(a_orig) ),
            listener_receiver( std::move(a_orig) ),
            f_status( a_orig.f_status ),
            f_name( std::move(a_orig.f_name) ),
            f_json_print( a_orig.f_json_print ),
            f_pretty_print( a_orig.f_pretty_print ),
            f_requests_keys( std::move(a_orig.f_requests_keys) ),
            f_alerts_keys( std::move(a_orig.f_alerts_keys) )
    {
        a_orig.f_status = status::nothing;
        a_orig.f_json_print = false;
        a_orig.f_pretty_print = false;
    }

    monitor::~monitor()
    {
        if( f_status >= status::listening )
        {
            this->cancel( dl_success().rc_value() );
            std::this_thread::sleep_for( std::chrono::milliseconds(1100) );
        }
        if( f_status > status::exchange_declared ) stop();
    }

    monitor& monitor::operator=( monitor&& a_orig )
    {
        core::operator=( std::move(a_orig) );
        listener::operator=( std::move(a_orig) );
        concurrent_receiver::operator=( std::move(a_orig) );
        f_status = a_orig.f_status;
        a_orig.f_status = status::nothing;
        f_name = std::move(a_orig.f_name);
        f_requests_keys = std::move(a_orig.f_requests_keys);
        f_alerts_keys = std::move(a_orig.f_alerts_keys);
        return *this;
    }

    bool monitor::start()
    {
        if( f_status != status::nothing )
        {
            LERROR( dlog, "Monitor is not in the right status to start" );
            return false;
        }

        if( f_requests_keys.empty() && f_alerts_keys.empty() )
        {
            LERROR( dlog, "No keys provided to monitor" );
            return false;
        }

        LINFO( dlog, "Connecting to <" << f_address << ":" << f_port << ">" );

        LDEBUG( dlog, "Opening channel for message monitor <" << f_name << ">" );
        f_channel = open_channel();
        if( ! f_channel ) return false;
        f_status = status::channel_created;

        if( ! setup_exchange( f_channel, f_requests_exchange ) ) return false;
        if( ! setup_exchange( f_channel, f_alerts_exchange ) ) return false;
        f_status = status::exchange_declared;

        LDEBUG( dlog, "Setting up queue for message monitor <" << f_name << ">" );
        if( ! setup_queue( f_channel, f_name ) ) return false;
        f_status = status::queue_declared;

        if( ! bind_keys() ) return false;
        f_status = status::queue_bound;

        f_consumer_tag = start_consuming( f_channel, f_name );
        if( f_consumer_tag.empty() ) return false;
        f_status = status::consuming;

        return true;
    }

    bool monitor::listen()
    {
        auto t_cancel_wrap = scarab::wrap_cancelable( *this );
        scarab::signal_handler::add_cancelable( t_cancel_wrap );

        if( f_status != status::consuming )
        {
            LERROR( dlog, "Monitor is not in the right status to listen" );
            return false;
        }

        f_status = status::listening;

        try
        {
            f_receiver_thread = std::thread( &concurrent_receiver::execute, this );

            if( ! listen_on_queue() )
            {
                throw dripline_error() << "Something went wrong while listening for messages";
            }

            f_receiver_thread.join();
        }
        catch( std::system_error& e )
        {
            LERROR( dlog, "Could not start the a thread due to a system error: " << e.what() );
            return false;
        }
        catch( dripline_error& e )
        {
            LERROR( dlog, "Dripline error while running monitor: " << e.what() );
            return false;
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Error while running monitor: " << e.what() );
            return false;
        }

        return true;

    }

    bool monitor::stop()
    {
        LINFO( dlog, "Stopping message monitor <" << f_name << ">" );

        if( f_status >= status::listening ) // listening
        {
            this->cancel( dl_success().rc_value() );
            f_status = status::consuming;
        }

        if( f_status >= status::queue_bound ) // queue_bound or consuming
        {
            if( ! stop_consuming( f_channel, f_consumer_tag ) ) return false;
            f_status = status::queue_bound;
        }

        if( f_status >= status::queue_declared ) // queue_declared or queue_bound
        {
            if( ! remove_queue( f_channel, f_name ) ) return false;
            f_status = status::exchange_declared;
        }

        return true;
    }

    bool monitor::bind_keys()
    {
        LDEBUG( dlog, "Binding request keys for message monitor <" << f_name << ">" );
        for( auto t_req_key_it = f_requests_keys.begin(); t_req_key_it != f_requests_keys.end(); ++t_req_key_it )
        {
            if( ! bind_key( f_channel, f_requests_exchange, f_name, *t_req_key_it ) ) return false;
        }

        LDEBUG( dlog, "Binding alerts keys for message monitor <" << f_name << ">" );
        for( auto t_al_key_it = f_alerts_keys.begin(); t_al_key_it != f_alerts_keys.end(); ++t_al_key_it )
        {
            if( ! bind_key( f_channel, f_alerts_exchange, f_name, *t_al_key_it ) ) return false;
        }

        return true;
    }

    bool monitor::listen_on_queue()
    {
        LINFO( dlog, "Listening for incoming messages on <" << f_name << ">" );

        while( ! is_canceled()  )
        {
            amqp_envelope_ptr t_envelope;
            core::post_listen_status t_post_listen_status = core::post_listen_status::unknown;
            core::listen_for_message( t_envelope, t_post_listen_status, f_channel, f_consumer_tag, f_listen_timeout_ms );

            if( f_canceled.load() )
            {
                LDEBUG( dlog, "Monitor <" << f_name << "> canceled" );
                return true;
            }

            if( t_post_listen_status == core::post_listen_status::timeout )
            {
                // we end up here every time the listen times out with no message received
                continue;
            }

            if( t_post_listen_status == core::post_listen_status::soft_error )
            {
                LWARN( dlog, "A soft error ocurred while listening for messages for monitor <" << f_name << ">.  The channel is still valid" );
                continue;
            }

            if( t_post_listen_status == core::post_listen_status::hard_error )
            {
                LERROR( dlog, "A hard error ocurred while listening for messages for monitor <" << f_name << ">.  The channel is no longer valid" );
                return false;
            }

            if( t_post_listen_status == core::post_listen_status::unknown )
            {
                LERROR( dlog, "An unknown status occurred while listening for messages for monitor <" << f_name << ">" );
                return false;
            }

            // remaining status is core::post_listen_status::message_received

            handle_message_chunk( t_envelope );

            if( f_canceled.load() )
            {
                LDEBUG( dlog, "Monitor <" << f_name << "> canceled" );
                return true;
            }
        }
        return true;
    }

    void monitor::submit_message( message_ptr_t a_message )
    {
        try
        {
            if( ! f_json_print && ! f_pretty_print )
            {
                if( a_message->is_request() )
                {
                    LPROG( dlog, *std::static_pointer_cast< msg_request >( a_message ) );
                    return;
                }
                if( a_message->is_reply() )
                {
                    LPROG( dlog, *std::static_pointer_cast< msg_reply >( a_message ) );
                    return;
                }
                if( a_message->is_alert() )
                {
                    LPROG( dlog, *std::static_pointer_cast< msg_alert >( a_message ) );
                    return;
                }
                LPROG( dlog, *a_message );
                return;
            }
            else
            {
                scarab::param_node t_encoding_options;
                if( f_pretty_print )
                {
                    t_encoding_options.add( "style", "pretty" );
                }
                std::string t_encoded_message = a_message->encode_full_message( 5000, t_encoding_options );
                LPROG( dlog, t_encoded_message );
                return;
            }
        }
        catch( dripline_error& e )
        {
            LERROR( dlog, "<" << f_name << "> Dripline exception caught while handling message: " << e.what() );
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "<" << f_name << "> Standard exception caught while sending reply: " << e.what() );
        }

        return;
    }

} /* namespace dripline */
