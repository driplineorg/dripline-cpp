/*
 * service.cc
 *
 *  Created on: Jan 5, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "service.hh"

#include "dripline_config.hh"
#include "dripline_exceptions.hh"
#include "service_config.hh"

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

    service::service( const scarab::param_node& a_config, const scarab::authentication& a_auth, const bool a_make_connection ) :
            scarab::cancelable(),
            core( a_config.has("dripline_mesh") ? a_config["dripline_mesh"].as_node() : dripline_config(), 
                  a_auth, a_make_connection ),
            endpoint( a_config.get_value( "name", "dlcpp_service" ) ),
            concurrent_receiver(),
            heartbeater( this ),
            scheduler<>(),
            f_auth( a_auth ),
            f_status( status::nothing ),
            f_restart_on_error( a_config.get_value( "restart_on_error", true ) ),
            f_enable_scheduling( a_config.get_value( "enable_scheduling", false ) ),
            f_id( generate_random_uuid() ),
            f_sync_children(),
            f_async_children(),
            f_broadcast_key( a_config.get_value( "broadcast_key", "broadcast" ) )
    {
        LDEBUG( dlog, "Service (cpp) created with config:\n" << a_config );
        // get more values from the config
        heartbeater::f_check_timeout_ms = a_config.get_value( "loop_timeout_ms", 1000 );
        // default of f_single_message_wait_ms is in the receiver class
        f_single_message_wait_ms = a_config.get_value( "message_wait_ms", f_single_message_wait_ms );
        // default of f_heartbeat_interval_s is in the heartbeater class
        f_heartbeat_interval_s = a_config.get_value( "heartbeat_interval_s", f_heartbeat_interval_s );
    }
/*
    service::service( const bool a_make_connection, const scarab::param_node& a_config, const scarab::authentication& a_auth ) :
            scarab::cancelable(),
            core( a_make_connection, a_config ),
            endpoint( "" ),
            listener_receiver(),
            heartbeater(),
            scheduler<>(),
            std::enable_shared_from_this< service >(),
            f_status( status::nothing ),
            f_enable_scheduling( a_config.get_value("enable-scheduling", false ) ),
            f_id( generate_random_uuid() ),
            f_sync_children(),
            f_async_children(),
            f_broadcast_key()
    {
    }
*/

    service::~service()
    {
        if( f_status >= status::listening )
        {
            this->cancel( dl_success().rc_value() );
            std::this_thread::sleep_for( std::chrono::milliseconds(1100) );
        }
        if( f_status > status::exchange_declared ) stop();
    }

    service& service::operator=( service&& a_orig )
    {
        cancelable::operator=( std::move(a_orig) );
        core::operator=( std::move(a_orig) );
        endpoint::operator=( std::move(a_orig));
        concurrent_receiver::operator=( std::move(a_orig) );
        heartbeater::operator=( std::move(a_orig) );
        scheduler<>::operator=( std::move(a_orig) );

        f_status = std::move( a_orig.f_status );
        f_restart_on_error = a_orig.f_restart_on_error;
        f_enable_scheduling = a_orig.f_enable_scheduling;
        f_id = std::move( a_orig.f_id );
        f_sync_children = std::move( a_orig.f_sync_children );
        f_async_children = std::move( a_orig.f_async_children );
        f_broadcast_key = std::move( a_orig.f_broadcast_key );

        return *this;
    }

    bool service::add_child( endpoint_ptr_t a_endpoint_ptr )
    {
        auto t_inserted = f_sync_children.insert( std::make_pair( a_endpoint_ptr->name(), a_endpoint_ptr ) );
        if( t_inserted.second )
        {
            a_endpoint_ptr->set_service( this );
        }
        else
        {
            LERROR( dlog, "Endpoint <" << a_endpoint_ptr->name() << " could not be added to service <" << f_name << ">" );
            return false;
        }
        return t_inserted.second;
    }

    bool service::add_async_child( endpoint_ptr_t a_endpoint_ptr )
    {
        elr_ptr_t t_elr_ptr = std::dynamic_pointer_cast< endpoint_listener_receiver >( a_endpoint_ptr );
        if( ! t_elr_ptr )
        {
            t_elr_ptr = std::make_shared< endpoint_listener_receiver >( a_endpoint_ptr );
        }
        auto t_inserted = f_async_children.insert( std::make_pair( a_endpoint_ptr->name(), t_elr_ptr ) );
        if( t_inserted.second )
        {
            a_endpoint_ptr->set_service( this );
        }
        else
        {
            LERROR( dlog, "Endpoint (async) <" << a_endpoint_ptr->name() << " could not be added to service <" << f_name << ">" );
            return false;
        }
        return t_inserted.second;
    }

    void service::run()
    {
        unsigned n_failures = 0;
        bool t_do_repeat = true; // start true so that we get into the repeat loop
        // Repeat loop for listening: we may call to listen_on_queue() multiple times
        while( t_do_repeat )
        {
            t_do_repeat = false; // set false because we'll only do the repeat based on the conditions below
            LINFO( dlog, "Starting the service" );
            if( ! start() ) throw dripline_error() << "There was a problem while starting the service (check for prior error messages)";
            try
            {
                LINFO( dlog, "Service started; now listening for messages" );
                if( ! listen() ) throw dripline_error() << "There was a problem while listening for messages (check for prior error messages)";
                n_failures = 0; // reset the number of failures to 0 once there's been a successful connection
            }
            catch( const dripline_error& e )
            {
                // We had an error while listening
                // Check whether or not we should try to connect again
                // 1. If we want to restart on error, and
                // 2. If the failure count is less than our threshold (2)
                ++n_failures;
                if( f_restart_on_error && n_failures < 2 )
                {
                    t_do_repeat = true;
                    // we'll report and then drop the exception to do the reconnect
                    LWARN( dlog, e.what() );
                    LWARN( dlog, "Will attempt to reconnect" );
                }
                else
                {
                    // if we're not going to connect again, and we had an error, propagate the error by rethrowing
                    LERROR( dlog, "Reached maximum number of reconnect attempts" );
                    throw;
                }
            }

            if( t_do_repeat )
            {
                reset_cancel();
            }
        }

        LINFO( dlog, "Stopping the service" );
        if( ! stop() ) throw dripline_error() << "There was a problem while stopping the service (check for prior error messages)";

        return;
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

        // fill in the link to this in endpoint because we couldn't do it in the constructor
        endpoint::f_service = this;
        heartbeater::f_service = this;

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

        // TODO (Phase 6): set up service topology, declare queues, bind routing keys
        // - declare service queue (f_name) and routing keys (f_name + ".#", f_broadcast_key + ".#")
        // - for each sync child, bind child_name + ".#" to the service queue
        // - call start_listening( f_vhost, topology, service_queue_handle, f_name )
        // - for each async child, declare its queue and call child->start_listening(...)
        f_status = status::consuming;

        return true;
    }

    bool service::listen()
    {
        if ( ! f_make_connection )
        {
            LWARN( dlog, "Should not listen for messages when make_connection is disabled" );
            return true;
        }

        f_status = status::listening;

        try
        {
            if( f_heartbeat_interval_s != 0 )
            {
                LINFO( dlog, "Starting heartbeat" );
                f_heartbeat_thread = std::thread( &heartbeater::execute, this, f_name, f_id, f_heartbeat_routing_key );
            }
            else
            {
                LINFO( dlog, "Heartbeat disabled" );
            }

            if( f_enable_scheduling )
            {
                LINFO( dlog, "Starting scheduler" );
                f_scheduler_thread = std::thread( &scheduler::execute, this );
            }
            else
            {
                LINFO( dlog, "Scheduler disabled" );
            }

            LINFO( dlog, "Starting receiver thread" );
            f_receiver_thread = std::thread( &concurrent_receiver::execute, this );

            if( ! f_async_children.empty() ) { LINFO( dlog, "Starting async children" ); }
            else { LDEBUG( dlog, "No async children to start" ); }
            for( auto& t_child_pair : f_async_children )
            {
                t_child_pair.second->receiver_thread() = std::thread( &concurrent_receiver::execute, t_child_pair.second.get() );
                // TODO (Phase 6): call t_child_pair.second->start_listening( f_vhost, topology, queue_handle, t_child_pair.first )
            }

            // Block until canceled
            while( ! is_canceled() )
            {
                std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
            }

            for( auto& t_child_pair : f_async_children )
            {
                if( t_child_pair.second->receiver_thread().joinable() )
                {
                    t_child_pair.second->receiver_thread().join();
                }
            }

            f_receiver_thread.join();

            if( f_heartbeat_thread.joinable() )
            {
                f_heartbeat_thread.join();
            }
            if( f_scheduler_thread.joinable() )
            {
                f_scheduler_thread.join();
            }
        }
        catch( std::system_error& e )
        {
            LERROR( dlog, "Could not start a thread due to a system error: " << e.what() );
            return false;
        }
        catch( dripline_error& e )
        {
            LERROR( dlog, "Dripline error while running service: " << e.what() );
            return false;
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "Error while running service: " << e.what() );
            return false;
        }

        return true;
    }

    bool service::stop()
    {
        LINFO( dlog, "Stopping service on <" << f_name << ">" );

        if( f_status >= status::listening )
        {
            this->cancel( dl_success().rc_value() );
            f_status = status::consuming;
        }

        stop_listening();

        for( auto& t_child_pair : f_async_children )
        {
            t_child_pair.second->stop_listening();
        }

        f_status = status::nothing;
        return true;
    }

    void service::submit_message( message_ptr_t a_message )
    {
        try
        {
            sort_message( a_message );
            return;
        }
        catch( dripline_error& e )
        {
            LERROR( dlog, "<" << f_name << "> Dripline exception caught while handling message: " << e.what() );
            throw;
        }
        catch( std::exception& e )
        {
            LERROR( dlog, "<" << f_name << "> Standard exception caught while handling message: " << e.what() );
            throw;
        }

        return;
    }

    reply_ptr_t service::on_request_message( request_ptr_t a_request )
    {
        std::string t_first_token( a_request->routing_key() );
        t_first_token = t_first_token.substr( 0, t_first_token.find_first_of('.') );
        LDEBUG( dlog, "First token in routing key: <" << t_first_token << ">" );

        if( t_first_token == f_name || t_first_token == f_broadcast_key )
        {
            // reply will be sent by endpoint::on_request_message
            return this->endpoint::on_request_message( a_request );
        }
        else
        {
            auto t_endpoint_itr = f_sync_children.find( t_first_token );
            if( t_endpoint_itr == f_sync_children.end() )
            {
                LERROR( dlog, "Did not find child endpoint called <" << t_first_token << ">" );
                throw dripline_error() << "Did not find child endpoint <" << t_first_token << ">";
            }

            // reply will be sent by endpoint::on_request_message or derived
            return t_endpoint_itr->second->on_request_message( a_request );
        }
    }

    void service::do_cancellation( int a_code )
    {
        LDEBUG( dlog, "Canceling service <" << f_name << ">" );
        for( async_map_t::iterator t_child_it = f_async_children.begin();
                t_child_it != f_async_children.end();
                ++t_child_it )
        {
            LDEBUG( dlog, "Canceling child endpoint <" << t_child_it->first << ">" );
            t_child_it->second->cancel( a_code );
        }
        return;
    }

} /* namespace dripline */
