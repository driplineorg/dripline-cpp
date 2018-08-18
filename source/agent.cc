/*
 * agent.cc
 *
 *  Created on: Jun 2, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS
#define SCARAB_API_EXPORTS

#include "agent.hh"

#include "dripline_constants.hh"
#include "dripline_version.hh"
#include "core.hh"
#include "uuid.hh"

#include "logger.hh"
#include "param_json.hh"
#include "path.hh"

#include <algorithm> // for min
#include <string>

// In Windows there's a preprocessor macro called uuid_t that conflicts with this typdef
#ifdef uuid_t
#undef uuid_t
#endif

using scarab::param;
using scarab::param_array;
using scarab::param_input_json;
using scarab::param_node;
using scarab::param_output_json;
using scarab::param_value;

namespace dripline
{
    LOGGER( dlog, "agent" );

    agent::agent() :
            f_config(),
            f_routing_key(),
            f_lockout_key( generate_nil_uuid() ),
            f_reply(),
            f_return( 0 )
    {
    }

    agent::~agent()
    {
    }
/*
    void agent::do_run( const scarab::param_node& a_node )
    {
        f_create_request_ptr = [this]()->request_ptr_t
                { return this->create_run_request(); };
        execute( a_node );
    }

    void agent::do_get( const scarab::param_node& a_node )
    {
        f_create_request_ptr = [this]()->request_ptr_t
                { return this->create_get_request(); };
        execute( a_node );
    }

    void agent::do_set( const scarab::param_node& a_node )
    {
        f_create_request_ptr = [this]()->request_ptr_t
                { return this->create_set_request(); };
        execute( a_node );
    }

    void agent::do_cmd( const scarab::param_node& a_node )
    {
        f_create_request_ptr = [this]()->request_ptr_t
                { return this->create_cmd_request(); };
        execute( a_node );
    }
*/

    void agent::sub_agent::execute( const param_node& a_node )
    {
        LINFO( dlog, "Creating request" );

        // create a copy of the config that will be pared down by removing expected elements
        param_node& t_config = f_agent->config();
        t_config = a_node;

        param_node t_amqp_node( std::move(t_config.remove( "amqp" )->as_node()) );
        unsigned t_timeout = t_amqp_node.get_value( "reply-timeout-ms", 10000 );

        core t_core( t_amqp_node );

        // pull the special CL arguments out of the configuration

        f_agent->routing_key() = t_config.get_value( "rk", f_agent->routing_key() );
        t_config.erase( "rk" );

        if( t_config.has( "lockout-key" ) )
        {
            bool t_lk_valid = true;
            f_agent->lockout_key() = dripline::uuid_from_string( t_config["lockout-key"]().as_string(), t_lk_valid );
            t_config.erase( "key" );
            if( ! t_lk_valid )
            {
                LERROR( dlog, "Invalid lockout key provided: <" << t_config.get_value( "lockout-key", "" ) << ">" );
                f_agent->set_return( RETURN_ERROR );
                return;
            }
        }

        param_node t_save_node;
        if( t_config.has( "save" ) )
        {
            t_save_node = t_config.remove( "save" )->as_node();
        }

        // create the request
        request_ptr_t t_request = this->create_request();

        if( t_request == NULL )
        {
            LERROR( dlog, "Unable to create request" );
            f_agent->set_return( RETURN_ERROR );
            return;
        }

        // now all that remains in f_config should be values to pass to the server as arguments to the request

        t_request->lockout_key() = f_agent->lockout_key();

        LINFO( dlog, "Connecting to AMQP broker" );

        //const param_node& t_broker_node = t_config["amqp"].as_node();

        LDEBUG( dlog, "Sending message w/ msgop = " << t_request->get_message_op() << " to " << t_request->routing_key() );

        rr_pkg_ptr t_receive_reply = t_core.send( t_request );

        if( ! t_receive_reply->f_successful_send )
        {
            LERROR( dlog, "Unable to send request" );
            f_agent->set_return( RETURN_ERROR );
            return;
        }

        if( ! t_receive_reply->f_consumer_tag.empty() )  // this indicates that the reply queue was created, and we've started consuming on it; we should wait for a reply
        {
            LINFO( dlog, "Waiting for a reply from the server; use ctrl-c to cancel" );

            // timed blocking call to wait for incoming message
            dripline::reply_ptr_t t_reply = t_core.wait_for_reply( t_receive_reply, t_timeout );

            if( t_reply )
            {
                LINFO( dlog, "Response received" );

                const param_node& t_payload = t_reply->payload();

                LINFO( dlog, "Response:\n" <<
                        "Return code: " << t_reply->get_return_code() << '\n' <<
                        "Return message: " << t_reply->return_msg() << '\n' <<
                        t_payload );

                // optionally save "master-config" from the response
                if( t_save_node.size() != 0 )
                {
                    if( t_save_node.has( "json" ) )
                    {
                        scarab::path t_save_filename( scarab::expand_path( t_save_node["json"]().as_string() ) );
                        if( t_payload.empty() )
                        {
                            LERROR( dlog, "Payload is not present" );
                        }
                        else
                        {
                            param_output_json t_output;
                            static param_node t_output_options;
                            if( t_output_options.empty() ) t_output_options.add( "style", (unsigned)param_output_json::k_pretty );
                            t_output.write_file( t_payload, t_save_filename.string(), t_output_options );
                        }
                    }
                    else
                    {
                        LERROR( dlog, "Save instruction did not contain a valid file type");
                    }

                }
            }
            else
            {
                LWARN( dlog, "Timed out waiting for reply" );
            }
            f_agent->set_reply( t_reply );
        }

        f_agent->set_return( RETURN_SUCCESS );

        return;
    }

    request_ptr_t agent::sub_agent_run::create_request()
    {
        param_node t_payload_node( f_agent->config() ); // copy of f_config, which should consist of only the request arguments

        return msg_request::create( t_payload_node, op_t::run, f_agent->routing_key(), "", message::encoding::json );
    }

    request_ptr_t agent::sub_agent_get::create_request()
    {
        param_node t_payload_node;

        if( f_agent->config().has( "value" ) )
        {
            param_array t_values_array;
            t_values_array.push_back( f_agent->config().remove( "value" ) );
            t_payload_node.add( "values", std::move(t_values_array) );
        }

        return msg_request::create( t_payload_node, op_t::get, f_agent->routing_key(), "", message::encoding::json );
    }

    request_ptr_t agent::sub_agent_set::create_request()
    {
        if( ! f_agent->config().has( "value" ) )
        {
            LERROR( dlog, "No \"value\" option given" );
            return NULL;
        }

        param_array t_values_array;
        t_values_array.push_back( f_agent->config().remove( "value" ) );

        param_node t_payload_node;
        t_payload_node.add( "values", std::move(t_values_array) );

        return msg_request::create( t_payload_node, op_t::set, f_agent->routing_key(), "", message::encoding::json );
    }

    request_ptr_t agent::sub_agent_cmd::create_request()
    {
        param_node t_payload_node;

        // for the load instruction, the instruction node should be replaced by the contents of the file specified
        if( f_agent->config().has( "load" ) )
        {
            if( ! f_agent->config()["load"].as_node().has( "json" ) )
            {
                LERROR( dlog, "Load instruction did not contain a valid file type");
                return NULL;
            }

            std::string t_load_filename( f_agent->config()["load"]["json"]().as_string() );
            param_input_json t_reader;
            scarab::param_ptr_t t_node_from_file = t_reader.read_file( t_load_filename );
            if( t_node_from_file == NULL || ! t_node_from_file->is_node() )
            {
                LERROR( dlog, "Unable to read JSON file <" << t_load_filename << ">" );
                return NULL;
            }

            t_payload_node.merge( t_node_from_file->as_node() );
            f_agent->config().erase( "load" );
        }

        // at this point, all that remains in f_config should be other options that we want to add to the payload node
        t_payload_node.merge( f_agent->config() ); // copy f_config

        return msg_request::create( t_payload_node, op_t::cmd, f_agent->routing_key(), "", message::encoding::json );
    }

} /* namespace dripline */
