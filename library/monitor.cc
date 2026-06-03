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

#include <chrono>
#include <thread>

LOGGER( dlog, "monitor" );

namespace dripline
{

    monitor::monitor( const scarab::param_node& a_config, const scarab::authentication& a_auth ) :
            scarab::cancelable(),
            core( a_config["dripline_mesh"].as_node(), a_auth ),
            message_dispatcher(),
            f_status( status::nothing ),
            f_name( std::string("monitor_") + string_from_uuid(generate_random_uuid()) ),
            f_json_print( false ),
            f_pretty_print( false ),
            f_requests_keys(),
            f_alerts_keys()
    {
        // get requests keys
        if( a_config.has( "request_keys" ) && a_config["request_keys"].is_array() )
        {
            const scarab::param_array& t_req_keys = a_config["request_keys"].as_array();
            f_requests_keys.reserve( t_req_keys.size() );
            for( auto t_it = t_req_keys.begin(); t_it != t_req_keys.end(); ++t_it )
            {
                LPROG( dlog, "Monitor <" << f_name << "> will monitor key <" << (*t_it)().as_string() << "> on the requests exchange" );
                f_requests_keys.push_back( (*t_it)().as_string() );
            }
        }

        if( a_config.has( "request_key" ) && a_config["request_key"].is_value() )
        {
            LPROG( dlog, "Monitor <" << f_name << "> will monitor key <" << a_config["request_key"]().as_string() << "> on the requests exchange" );
            f_requests_keys.push_back( a_config["request_key"]().as_string() );
        }

        // get alerts keys
        if( a_config.has( "alert_keys" ) && a_config["alert_keys"].is_array() )
        {
            const scarab::param_array& t_req_keys = a_config["alert_keys"].as_array();
            f_requests_keys.reserve( t_req_keys.size() );
            for( auto t_it = t_req_keys.begin(); t_it != t_req_keys.end(); ++t_it )
            {
                LPROG( dlog, "Monitor <" << f_name << "> will monitor key <" << (*t_it)().as_string() << "> on the alerts exchange" );
                f_alerts_keys.push_back( (*t_it)().as_string() );
            }
        }

        if( a_config.has( "alert_key" ) && a_config["alert_key"].is_value() )
        {
            LPROG( dlog, "Monitor <" << f_name << "> will monitor key <" << a_config["alert_key"]().as_string() << "> on the alerts exchange" );
            f_alerts_keys.push_back( a_config["alert_key"]().as_string() );
        }
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

        try
        {
            open_connection();
        }
        catch( connection_error& e )
        {
            LERROR( dlog, "Unable to connect to the broker: " << e.what() );
            return false;
        }
        f_status = status::channel_created;

        try
        {
            using namespace BloombergLP;

            // Monitor queue: ephemeral (auto-delete, non-durable); f_name already contains a UUID
            rmqa::Topology t_topo;
            auto t_monitor_queue = t_topo.addQueue( bsl::string(f_name), rmqt::AutoDelete::ON, rmqt::Durable::OFF );
            if( ! f_requests_keys.empty() )
            {
                auto t_req_ex = t_topo.addExchange( bsl::string(f_requests_exchange), rmqt::ExchangeType::TOPIC );
                for( const auto& t_key : f_requests_keys )
                {
                    t_topo.bind( t_req_ex, t_monitor_queue, bsl::string(t_key) );
                }
            }
            if( ! f_alerts_keys.empty() )
            {
                auto t_alerts_ex = t_topo.addExchange( bsl::string(f_alerts_exchange), rmqt::ExchangeType::TOPIC );
                for( const auto& t_key : f_alerts_keys )
                {
                    t_topo.bind( t_alerts_ex, t_monitor_queue, bsl::string(t_key) );
                }
            }
            start_listening( f_vhost, t_topo, t_monitor_queue, f_name );
        }
        catch( connection_error& e )
        {
            LERROR( dlog, "Unable to set up monitor topology: " << e.what() );
            return false;
        }
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
            // Block until canceled
            while( ! is_canceled() )
            {
                std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
            }
        }
        catch( std::system_error& e )
        {
            LERROR( dlog, "Could not start a thread due to a system error: " << e.what() );
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

        if( f_status >= status::listening )
        {
            this->cancel( dl_success().rc_value() );
            f_status = status::consuming;
        }

        stop_listening();

        f_status = status::nothing;
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
            LERROR( dlog, "<" << f_name << "> Standard exception caught while handling message: " << e.what() );
        }

        return;
    }

} /* namespace dripline */
