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
#include "dripline_error.hh"
#include "dripline_version.hh"
#include "service.hh"
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

using std::string;

using scarab::param_array;
using scarab::param_input_json;
using scarab::param_node;
using scarab::param_output_json;

namespace dripline
{
    LOGGER( dlog, "agent" );

    agent::agent( const param_node& a_node ) :
            f_config( a_node ),
            f_return( 0 )
    {
    }

    agent::~agent()
    {
    }

    void agent::execute()
    {
        LINFO( dlog, "Creating request" );

        // pull the special CL arguments out of the configuration

        std::string t_routing_key( f_config.get_value( "rk", "my_queue" ) );
        f_config.erase( "rk" );

        std::string t_lockout_key_str( f_config.get_value( "lockout-key", "" ) );
        f_config.erase( "key" );
        bool t_lk_valid = true;
        dripline::uuid_t t_lockout_key = dripline::uuid_from_string( t_lockout_key_str, t_lk_valid );
        if( ! t_lk_valid )
        {
            LERROR( dlog, "Invalid lockout key provided: <" << t_lockout_key_str << ">" );
            f_return = RETURN_ERROR;
            return;
        }

        param_node t_save_node;
        if( f_config.has( "save" ) )
        {
            t_save_node = *(f_config.node_at( "save" ));
        }
        f_config.erase( "save" );

        request_ptr_t t_request;
        if( f_config.has( "run" ) )
        {
            f_config.erase( "run" );
            t_request = create_run_request( t_routing_key );
        }
        else if( f_config.has( "get" ) )
        {
            f_config.erase( "get" );
            t_request = create_get_request( t_routing_key );
        }
        else if( f_config.has( "set" ) )
        {
            f_config.erase( "set" );
            t_request = create_set_request( t_routing_key );
        }
        else if( f_config.has( "cmd" ) )
        {
            f_config.erase( "cmd" );
            t_request = create_cmd_request( t_routing_key );
        }
        else
        {
            LERROR( dlog, "Unknown or missing request type" );
            f_return = RETURN_ERROR;
            return;
        }

        if( t_request == NULL )
        {
            LERROR( dlog, "Unable to create request" );
            f_return = RETURN_ERROR;
            return;
        }

        // now all that remains in f_config should be values to pass to the server as arguments to the request

        t_request->lockout_key() = t_lockout_key;

        LINFO( dlog, "Connecting to AMQP broker" );

        param_node& t_broker_node = f_config.remove( "amqp" )->as_node();

        dripline::service t_service( t_broker_node.get_value( "broker" ),
                                     t_broker_node.get_value< unsigned >( "broker-port" ),
                                     t_broker_node.get_value( "exchange" ),
                                     "", ".project8_authentications.json" );

        LDEBUG( dlog, "Sending message w/ msgop = " << t_request->get_message_op() << " to " << t_request->routing_key() );

        dripline::service::rr_pkg_ptr t_receive_reply = t_service.send( t_request );

        if( ! t_receive_reply->f_successful_send )
        {
            LERROR( dlog, "Unable to send request" );
            f_return = RETURN_ERROR;
            return;
        }

        if( ! t_receive_reply->f_consumer_tag.empty() )  // this indicates that the reply queue was created, and we've started consuming on it; we should wait for a reply
        {
            LINFO( dlog, "Waiting for a reply from the server; use ctrl-c to cancel" );

            // timed blocking call to wait for incoming message
            dripline::reply_ptr_t t_reply = t_service.wait_for_reply( t_receive_reply, t_broker_node.get_value< int >( "reply-timeout-ms" ) );

            if( t_reply )
            {
                LINFO( dlog, "Response received" );

                const param_node* t_payload = &( t_reply->get_payload() );

                LINFO( dlog, "Response:\n" <<
                        "Return code: " << t_reply->get_return_code() << '\n' <<
                        "Return message: " << t_reply->return_msg() << '\n' <<
                        *t_payload );

                // optionally save "master-config" from the response
                if( t_save_node.size() != 0 )
                {
                    if( t_save_node.has( "json" ) )
                    {
                        scarab::path t_save_filename( scarab::expand_path( t_save_node.get_value( "json" ) ) );
                        const param_node* t_master_config_node = t_payload;
                        if( t_master_config_node == NULL )
                        {
                            LERROR( dlog, "Payload is not present" );
                        }
                        else
                        {
                            param_output_json::write_file( *t_master_config_node, t_save_filename.string(), param_output_json::k_pretty );
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
        }

        f_return = RETURN_SUCCESS;

        return;
    }

    int agent::get_return()
    {
        return f_return;
    }

    request_ptr_t agent::create_run_request( const std::string& a_routing_key )
    {
        if( ! f_config.has( "file" ) )
        {
            LERROR( dlog, "The filename to be saved must be specified with the \"file\" option" );
            return NULL;
        }

        param_node* t_payload_node = new param_node( f_config ); // copy of f_config, which should consist of only the request arguments

        return msg_request::create( t_payload_node, op_t::run, a_routing_key, "", message::encoding::json );
    }

    request_ptr_t agent::create_get_request( const std::string& a_routing_key )
    {
        param_node* t_payload_node = new param_node();

        if( f_config.has( "value" ) )
        {
            param_array* t_values_array = new param_array();
            t_values_array->push_back( f_config.remove( "value" ) );
            t_payload_node->add( "values", t_values_array );
        }

        return msg_request::create( t_payload_node, op_t::get, a_routing_key, "", message::encoding::json );
    }

    request_ptr_t agent::create_set_request( const std::string& a_routing_key )
    {
        if( ! f_config.has( "value" ) )
        {
            LERROR( dlog, "No \"value\" option given" );
            return NULL;
        }

        param_array* t_values_array = new param_array();
        t_values_array->push_back( f_config.remove( "value" ) );

        param_node* t_payload_node = new param_node();
        t_payload_node->add( "values", t_values_array );

        return msg_request::create( t_payload_node, op_t::set, a_routing_key, "", message::encoding::json );
    }

    request_ptr_t agent::create_cmd_request( const std::string& a_routing_key )
    {
        param_node* t_payload_node = new param_node();

        // for the load instruction, the instruction node should be replaced by the contents of the file specified
        if( f_config.has( "load" ) )
        {
            if( ! f_config.node_at( "load" )->has( "json" ) )
            {
                LERROR( dlog, "Load instruction did not contain a valid file type");
                delete t_payload_node;
                return NULL;
            }

            string t_load_filename( f_config.node_at( "load" )->get_value( "json" ) );
            param_node* t_node_from_file = param_input_json::read_file( t_load_filename );
            if( t_node_from_file == NULL )
            {
                LERROR( dlog, "Unable to read JSON file <" << t_load_filename << ">" );
                delete t_payload_node;
                return NULL;
            }

            t_payload_node->merge( *t_node_from_file );
            f_config.erase( "load" );
        }

        // at this point, all that remains in f_config should be other options that we want to add to the payload node
        t_payload_node->merge( f_config ); // copy f_config

        return msg_request::create( t_payload_node, op_t::cmd, a_routing_key, "", message::encoding::json );
    }

} /* namespace dripline */
