/*
 * message.cc
 *
 *  Created on: Jul 9, 2015
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "message.hh"

#include "dripline_constants.hh"
#include "dripline_error.hh"
#include "dripline_version.hh"
#include "return_codes.hh"

#include "logger.hh"
#include "param_json.hh"
#include "return_codes.hh"
#include "time.hh"
#include "version_wrapper.hh"

#include <map>

using std::shared_ptr;
using std::make_shared;
using std::string;

using scarab::param;
using scarab::param_node;
using scarab::param_value;
using scarab::param_input_json;
using scarab::param_output_json;
using scarab::param_ptr_t;

using std::string;

namespace dripline
{

    LOGGER( dlog, "message" );

    //***********
    // Message
    //***********

    message::message() :
            f_routing_key(),
            f_correlation_id(),
            f_reply_to(),
            f_encoding( encoding::json ),
            f_timestamp(),
            f_sender_package( "N/A" ),
            f_sender_exe( "N/A" ),
            f_sender_version( "N/A" ),
            f_sender_commit( "N/A" ),
            f_sender_hostname( "N/A" ),
            f_sender_username( "N/A" ),
            f_sender_info(),
            f_specifier(),
            f_payload( new param() )
    {
        // make sure the sender_info node is filled out correctly
        f_sender_info.add( "package", param_value( "N/A" ) );
        f_sender_info.add( "exe", param_value( "N/A" ) );
        f_sender_info.add( "version", param_value( "N/A" ) );
        f_sender_info.add( "commit", param_value( "N/A" ) );
        f_sender_info.add( "hostname", param_value( "N/A" ) );
        f_sender_info.add( "username", param_value( "N/A" ) );
        f_sender_info.add( "service_name", param_value( "N/A" ) );

        // set the sender_info correctly for the server software
        scarab::version_wrapper* t_version = scarab::version_wrapper::get_instance();
        set_sender_commit( t_version->commit() );
        set_sender_version( t_version->version_str() );
        set_sender_package( t_version->package() );
        set_sender_exe( t_version->exe_name() );
        set_sender_hostname( t_version->hostname() );
        set_sender_username( t_version->username() );
        set_sender_service_name( "unknown" );
    }

    message::~message()
    {
    }

    message_ptr_t message::process_envelope( amqp_envelope_ptr a_envelope )
    {
        if( ! a_envelope )
        {
            throw dripline_error() << "Empty envelope received";
        }
        scarab::param_ptr_t t_msg;
        encoding t_encoding;
        if( a_envelope->Message()->ContentEncoding() == "application/json" )
        {
            t_encoding = encoding::json;
            param_input_json t_input;
            t_msg = t_input.read_string( a_envelope->Message()->Body() );
            if( ! t_msg )
            {
                throw dripline_error() << "Message body could not be parsed; skipping request";
            }
            if( ! t_msg->is_node() )
            {
                throw dripline_error() << "Message did not parse into a node";
            }
        }
        else
        {
            throw dripline_error() << "Unable to parse message with content type <" << a_envelope->Message()->ContentEncoding() << ">";
        }

        param_node& t_msg_node = t_msg->as_node();
        scarab::param_ptr_t t_payload( t_msg_node["payload"].clone() );

        string t_routing_key = a_envelope->RoutingKey();

        // This block is for dripline v2 compatibility
        size_t t_sep_pos = t_routing_key.find( '.' );
        string t_specifier;
        if( t_sep_pos != t_routing_key.npos )
        {
            t_specifier = t_routing_key.substr( t_sep_pos + 1 );
            t_routing_key = t_routing_key.substr( 0, t_sep_pos );
        }
        LDEBUG( dlog, "Dripline v2 compatibility: splitting the AMQP routing key into the \"routing key\" = <" << t_routing_key << "> and \"specifier\" = <" << t_specifier << ">");
        t_msg_node.add( "specifier", t_specifier );

        LDEBUG( dlog, "Processing message:\n" <<
                 "Routing key: " << t_routing_key <<
                 t_msg_node );

        message_ptr_t t_message;
        switch( to_msg_t( t_msg_node["msgtype"]().as_uint() ) )
        {
            case msg_t::request:
            {
                request_ptr_t t_request = msg_request::create(
                        std::move(t_payload),
                        to_op_t( t_msg_node.get_value< uint32_t >( "msgop", to_uint( op_t::unknown ) ) ),
                        t_routing_key,
                        t_msg_node.get_value( "specifier", "" ),
                        a_envelope->Message()->ReplyTo(),
                        t_encoding);

                bool t_lockout_key_valid = true;
                t_request->lockout_key() = uuid_from_string( t_msg_node.get_value( "lockout_key", "" ), t_lockout_key_valid );
                t_request->set_lockout_key_valid( t_lockout_key_valid );

                t_message = t_request;
                break;
            }
            case msg_t::reply:
            {
                reply_ptr_t t_reply = msg_reply::create(
                        t_msg_node["retcode"]().as_uint(),
                        t_msg_node.get_value( "return_msg", "" ),
                        std::move(t_payload),
                        t_routing_key,
                        t_msg_node.get_value( "specifier", "" ),
                        t_encoding);

                t_message = t_reply;
                break;
            }
            case msg_t::alert:
            {
                alert_ptr_t t_alert = msg_alert::create(
                        std::move(t_payload),
                        t_routing_key,
                        t_msg_node.get_value( "specifier", "" ),
                        t_encoding);

                t_message = t_alert;
                break;
            }
            default:
            {
                throw dripline_error() << "Message received with unhandled type: " << t_msg_node["msgtype"]().as_uint();
                break;
            }
        }

        // set message fields
        t_message->correlation_id() = a_envelope->Message()->CorrelationId();
        t_message->timestamp() = t_msg_node.get_value( "timestamp", "" );

        t_message->set_sender_info( t_msg_node["sender_info"].as_node() );

        if( t_msg_node.has( "payload" ) )
        {
            t_message->payload() = *t_msg_node["payload"].clone();
        }
        else
        {
            t_message->payload() = param();
        }

        return t_message;
    }

    amqp_message_ptr message::create_amqp_message() const
    {
        string t_body;
        if( ! encode_message_body( t_body ) )
        {
            LERROR( dlog, "Unable to encode message body" );
            return amqp_message_ptr();
        }

        amqp_message_ptr t_message = AmqpClient::BasicMessage::Create( t_body );
        t_message->ContentEncoding( interpret_encoding() );
        t_message->CorrelationId( f_correlation_id );
        t_message->ReplyTo( f_reply_to );
        this->derived_modify_amqp_message( t_message );

        return t_message;
    }

    bool message::encode_message_body( std::string& a_body ) const
    {
        param_node t_body_node;
        t_body_node.add( "msgtype", to_uint( message_type() ) );
        // Dripline v2 compatibility: the specifier is not used
        // For sending messages it should be inlcluded directly in the routing key
        //t_body_node.add( "specifier", f_specifier.to_string() );
        if( ! f_specifier.empty() )
        {
            throw dripline_error() << "For dripline v2 compatibility the specifier is not used.  Please include the specifier information directly in the routing key";
        }
        t_body_node.add( "timestamp", scarab::get_formatted_now() );
        t_body_node.add( "sender_info", f_sender_info );
        t_body_node.add( "payload", *f_payload );

        if( ! this->derived_modify_message_body( t_body_node ) )
        {
            LERROR( dlog, "Something went wrong in the derived-class modify_body_message function" );
            return false;
        }

        switch( f_encoding )
        {
            case encoding::json:
            {
                param_output_json t_output;
                if( ! t_output.write_string( t_body_node, a_body ) )
                {
                    LERROR( dlog, "Could not convert message body to string" );
                    return false;
                }
                return true;
                break;
            }
            default:
                LERROR( dlog, "Cannot encode using <" << interpret_encoding() << "> (" << f_encoding << ")" );
                return false;
                break;
        }
        // should not get here
        return false;
    }

    string message::interpret_encoding() const
    {
        switch( f_encoding )
        {
            case encoding::json:
                return std::string( "application/json" );
                break;
            default:
                return std::string( "Unknown" );
        }
    }



    //***********
    // Request
    //***********

    msg_request::msg_request() :
            message(),
            f_lockout_key( generate_nil_uuid() ),
            f_lockout_key_valid( true ),
            f_message_op( op_t::unknown )
    {
        f_correlation_id = string_from_uuid( generate_random_uuid() );
    }

    msg_request::~msg_request()
    {

    }

    request_ptr_t msg_request::create( param_ptr_t a_payload, op_t a_msg_op, const std::string& a_routing_key, const std::string& a_specifier, const std::string& a_reply_to, message::encoding a_encoding )
    {
        request_ptr_t t_request = make_shared< msg_request >();
        t_request->set_payload( std::move(a_payload) );
        t_request->set_message_op( a_msg_op );
        t_request->routing_key() = a_routing_key;
        t_request->parsed_specifier() = a_specifier;
        t_request->reply_to() = a_reply_to;
        t_request->set_encoding( a_encoding );
        return t_request;
    }

    msg_t msg_request::s_message_type = msg_t::request;

    msg_t msg_request::message_type() const
    {
        return msg_request::s_message_type;
    }


    //*********
    // Reply
    //*********

    msg_reply::msg_reply() :
            message(),
            f_return_code( dl_success::value ),
            f_return_msg(),
            f_return_buffer()
    {
    }

    msg_reply::~msg_reply()
    {

    }

    reply_ptr_t msg_reply::create( const return_code& a_retcode, const std::string& a_ret_msg, param_ptr_t a_payload, const std::string& a_routing_key, const std::string& a_specifier, message::encoding a_encoding )
    {
        return msg_reply::create( a_retcode.rc_value(), a_ret_msg, std::move(a_payload), a_routing_key, a_specifier, a_encoding );
    }

    reply_ptr_t msg_reply::create( unsigned a_retcode_value, const std::string& a_ret_msg, param_ptr_t a_payload, const std::string& a_routing_key, const std::string& a_specifier, message::encoding a_encoding )
    {
        reply_ptr_t t_reply = make_shared< msg_reply >();
        t_reply->set_return_code( a_retcode_value );
        t_reply->return_msg() = a_ret_msg;
        t_reply->set_payload( std::move(a_payload) );
        t_reply->routing_key() = a_routing_key;
        t_reply->parsed_specifier() = a_specifier;
        t_reply->set_encoding( a_encoding );
        return t_reply;
    }

    msg_t msg_reply::s_message_type = msg_t::reply;

    msg_t msg_reply::message_type() const
    {
        return msg_reply::s_message_type;
    }


    //*********
    // Alert
    //*********

    alert_ptr_t msg_alert::create( param_ptr_t a_payload, const std::string& a_routing_key, const std::string& a_specifier, message::encoding a_encoding )
    {
        alert_ptr_t t_alert = make_shared< msg_alert >();
        t_alert->set_payload( std::move(a_payload) );
        t_alert->routing_key() = a_routing_key;
        t_alert->parsed_specifier() = a_specifier;
        t_alert->set_encoding( a_encoding );
        return t_alert;
    }

    msg_alert::msg_alert() :
            message()
    {
        f_correlation_id = string_from_uuid( generate_random_uuid() );
    }

    msg_alert::~msg_alert()
    {

    }

    msg_t msg_alert::s_message_type = msg_t::alert;

    msg_t msg_alert::message_type() const
    {
        return msg_alert::s_message_type;
    }



    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, message::encoding a_enc )
    {
        static std::map< message::encoding, string > s_enc_strings = { { message::encoding::json, "json" } };
        return a_os << s_enc_strings[ a_enc ];
    }

    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, const message& a_message )
    {
        std::string t_msg;
        a_message.encode_message_body( t_msg );
        return a_os << t_msg;
    }


} /* namespace dripline */
