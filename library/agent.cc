/*
 * agent.cc
 *
 *  Created on: Jun 2, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS
#define SCARAB_API_EXPORTS

#include "agent.hh"

#include "agent_config.hh"
#include "dripline_constants.hh"
#include "dripline_version.hh"
#include "core.hh"
#include "uuid.hh"

#include "logger.hh"
#include "param_codec.hh"
#include "path.hh"

#include <algorithm> // for min
#include <string>

// In Windows there's a preprocessor macro called uuid_t that conflicts with this typdef
#ifdef uuid_t
#undef uuid_t
#endif

using scarab::param;
using scarab::param_array;
using scarab::param_node;
using scarab::param_ptr_t;
using scarab::param_value;

namespace dripline
{
    LOGGER( dlog, "agent" );

    agent::agent() :
            scarab::main_app(),
            //f_config(),
            f_routing_key(),
            f_specifier(),
            f_lockout_key( generate_nil_uuid() ),
            f_reply(),
            f_return( 0 )
    {
        require_subcommand(); // we use subcommands here
        fallthrough(); // pass any options not handled by the subcommands to the main app (this)

        // subcommands
        scarab::app* t_agent_run = add_subcommand( "run", "Send an OP_RUN request" );
        t_agent_run->callback(
                [this]() { this->execute< agent::sub_agent_run >(); }
        );

        scarab::app* t_agent_get = add_subcommand( "get", "Send an OP_GET request" );
        t_agent_get->callback(
                [this]() { this->execute< agent::sub_agent_get >(); }
        );

        scarab::app* t_agent_set = add_subcommand( "set", "Send an OP_SET request" );
        t_agent_set->callback(
                [this]() { this->execute< agent::sub_agent_set >(); }
        );

        scarab::app* t_agent_cmd = add_subcommand( "cmd", "Send an OP_CMD request" );
        t_agent_cmd->callback(
                [this]() { this->execute< agent::sub_agent_cmd>(); }
        );

        f_default_config = agent_config(); // load the default config

        add_option( "-b,--broker", [&](std::vector< std::string > args){ set_opt_broker( args[0] ); return true; }, "Set the dripline broker" );
    }

    agent::~agent()
    {
    }

    void agent::sub_agent::execute( const scarab::param_node& a_config )
    {
        LINFO( dlog, "Creating request" );

        // create a copy of the config that will be pared down by removing expected elements
        param_node t_config( a_config ); //f_agent->config();
        //t_config = a_node;

        param_node t_amqp_node;
        unsigned t_timeout = 0;
        if( t_config.has( "amqp" ) )
        {
            t_amqp_node = std::move(t_config.remove( "amqp" )->as_node());
            t_timeout = t_amqp_node.get_value( "reply-timeout-ms", 10000 );
        }

        core t_core( t_amqp_node );

        // pull the special CL arguments out of the configuration

        f_agent->routing_key() = t_config.get_value( "rk", f_agent->routing_key() );
        t_config.erase( "rk" );

        f_agent->specifier() = t_config.get_value( "sp", f_agent->specifier() );
        t_config.erase( "sp" );

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

        std::string t_save_filename( t_config.get_value( "save", "" ) );
        t_config.erase( "save" );

        // create the request
        request_ptr_t t_request = this->create_request( t_config );

        if( ! t_request )
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

                const param& t_payload = t_reply->payload();

                LINFO( dlog, "Response:\n" <<
                        "Return code: " << t_reply->get_return_code() << '\n' <<
                        "Return message: " << t_reply->return_msg() << '\n' <<
                        t_payload );

                if( ! t_save_filename.empty() && ! t_payload.is_null() )
                {
                    scarab::param_translator t_translator;
                    if( ! t_translator.write_file( t_payload, t_save_filename ) )
                    {
                        LERROR( dlog, "Unable to write out payload" );
                        f_agent->set_return( RETURN_ERROR );
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

    request_ptr_t agent::sub_agent_run::create_request( scarab::param_node& a_config )
    {
        // copy of a_config, which should consist of only the request arguments
        param_ptr_t t_payload_ptr( new param_node( a_config ) );

        return msg_request::create( std::move(t_payload_ptr), op_t::run, f_agent->routing_key(), f_agent->specifier(), "", message::encoding::json );
    }

    request_ptr_t agent::sub_agent_get::create_request( scarab::param_node& a_config )
    {
        param_ptr_t t_payload_ptr( new param_node() );

        if( a_config.has( "value" ) )
        {
            param_array t_values_array;
            t_values_array.push_back( a_config.remove( "value" ) );
            t_payload_ptr->as_node().add( "values", t_values_array );
        }

        return msg_request::create( std::move(t_payload_ptr), op_t::get, f_agent->routing_key(), f_agent->specifier(), "", message::encoding::json );
    }

    request_ptr_t agent::sub_agent_set::create_request( scarab::param_node& a_config )
    {
        if( ! a_config.has( "value" ) )
        {
            LERROR( dlog, "No \"value\" option given" );
            return nullptr;
        }

        param_array t_values_array;
        t_values_array.push_back( a_config.remove( "value" ) );

        param_ptr_t t_payload_ptr( new param_node() );
        t_payload_ptr->as_node().add( "values", t_values_array );

        return msg_request::create( std::move(t_payload_ptr), op_t::set, f_agent->routing_key(), f_agent->specifier(), "", message::encoding::json );
    }

    request_ptr_t agent::sub_agent_cmd::create_request( scarab::param_node& a_config )
    {
        param_ptr_t t_payload_ptr( new param_node() );
        param_node& t_payload_node = t_payload_ptr->as_node();

        // for the load instruction, the instruction node should be replaced by the contents of the file specified
        if( a_config.has( "load" ) )
        {
            if( ! a_config["load"].as_node().has( "json" ) )
            {
                LERROR( dlog, "Load instruction did not contain a valid file type");
                return nullptr;
            }

            std::string t_load_filename( a_config["load"]().as_string() );
            scarab::param_translator t_translator;
            scarab::param_ptr_t t_node_from_file = t_translator.read_file( t_load_filename );
            if( t_node_from_file == nullptr || ! t_node_from_file->is_node() )
            {
                LERROR( dlog, "Unable to read JSON file <" << t_load_filename << ">" );
                return nullptr;
            }

            t_payload_node.merge( t_node_from_file->as_node() );
            a_config.erase( "load" );
        }

        // at this point, all that remains in a_config should be other options that we want to add to the payload node
        t_payload_node.merge( a_config ); // copy a_config

        return msg_request::create( std::move(t_payload_ptr), op_t::cmd, f_agent->routing_key(), f_agent->specifier(), "", message::encoding::json );
    }

} /* namespace dripline */
