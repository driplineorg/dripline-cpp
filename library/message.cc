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
#include "map_at_default.hh"
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
            f_specifier(),
            f_payload( new param() )
    {
        // set the sender_info correctly for the server software
        scarab::version_wrapper* t_version = scarab::version_wrapper::get_instance();
        f_sender_commit = t_version->commit();
        f_sender_version = t_version->version_str();
        f_sender_package = t_version->package();
        f_sender_exe = t_version->exe_name();
        f_sender_hostname = t_version->hostname();
        f_sender_username = t_version->username();
        f_sender_service_name = "unknown";
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
        return message::process_message( a_envelope->Message(), a_envelope->RoutingKey() );
    }

    message_ptr_t message::process_message( amqp_message_ptr a_message, const std::string& a_routing_key )
    {
        scarab::param_ptr_t t_payload;
        encoding t_encoding;
        if( a_message->ContentEncoding() == "application/json" )
        {
            t_encoding = encoding::json;
            param_input_json t_input;
            t_payload = t_input.read_string( a_message->Body() );
            if( ! t_payload )
            {
                throw dripline_error() << "Message body could not be parsed; skipping request";
            }
            if( ! t_payload->is_node() )
            {
                throw dripline_error() << "Payload did not parse into a node";
            }
        }
        else
        {
            throw dripline_error() << "Unable to parse message with content type <" << a_message->ContentEncoding() << ">";
        }

        using AmqpClient::Table;
        using AmqpClient::TableEntry;
        using AmqpClient::TableValue;
        Table t_properties = a_message->HeaderTable();

        LDEBUG( dlog, "Processing message:\n" <<
                 "Routing key: " << a_routing_key <<
                 "Payload: " << t_payload->to_string() );

        using scarab::at;

        message_ptr_t t_message;
        msg_t t_msg_type = to_msg_t( at( t_properties, std::string("msgtype"), TableValue(to_uint(msg_t::unknown)) ).GetUint32() );
        switch( t_msg_type )
        {
            case msg_t::request:
            {
                request_ptr_t t_request = msg_request::create(
                        std::move(t_payload),
                        to_op_t( at( t_properties, std::string("msgop"), TableValue(to_uint(op_t::unknown)) ).GetUint32() ),
                        a_routing_key,
                        at( t_properties, std::string("specifier"), TableValue("") ).GetString(),
                        a_message->ReplyTo(),
                        t_encoding);

                bool t_lockout_key_valid = true;
                t_request->lockout_key() = uuid_from_string( at( t_properties, std::string("lockout_key"), TableValue("") ).GetString(), t_lockout_key_valid );
                t_request->set_lockout_key_valid( t_lockout_key_valid );

                t_message = t_request;
                break;
            }
            case msg_t::reply:
            {
                reply_ptr_t t_reply = msg_reply::create(
                        at( t_properties, std::string("retcode"), TableValue(999U) ).GetUint32(),
                        at( t_properties, std::string("return_msg"), TableValue("") ).GetString(),
                        std::move(t_payload),
                        a_routing_key,
                        at( t_properties, std::string("specifier"), TableValue("") ).GetString(),
                        t_encoding);

                t_message = t_reply;
                break;
            }
            case msg_t::alert:
            {
                alert_ptr_t t_alert = msg_alert::create(
                        std::move(t_payload),
                        a_routing_key,
                        at( t_properties, std::string("specifier"), TableValue("") ).GetString(),
                        t_encoding);

                t_message = t_alert;
                break;
            }
            default:
            {
                throw dripline_error() << "Message received with unhandled type: " << t_msg_type;
                break;
            }
        }

        // set message fields
        t_message->correlation_id() = a_message->CorrelationId();
        t_message->timestamp() = at( t_properties, std::string("timestamp"), TableValue("") ).GetString();

        Table t_sender_info = at( t_properties, std::string("sender_info"), TableValue(Table()) ).GetTable();
        scarab::param_node t_sender_info_node;
        t_sender_info_node.add( "package", at( t_sender_info, std::string("package"), TableValue("N/A") ).GetString() );
        t_sender_info_node.add( "exe", at( t_sender_info, std::string("exe"), TableValue("N/A") ).GetString() );
        t_sender_info_node.add( "version", at( t_sender_info, std::string("version"), TableValue("N/A") ).GetString() );
        t_sender_info_node.add( "commit", at( t_sender_info, std::string("commit"), TableValue("N/A") ).GetString() );
        t_sender_info_node.add( "hostname", at( t_sender_info, std::string("hostname"), TableValue("N/A") ).GetString() );
        t_sender_info_node.add( "username", at( t_sender_info, std::string("username"), TableValue("N/A") ).GetString() );
        t_sender_info_node.add( "service_name", at( t_sender_info, std::string("service_name"), TableValue("N/A") ).GetString() );
        t_message->set_sender_info( t_sender_info_node );

        t_message->payload() = *t_payload;

        return t_message;
    }

    amqp_message_ptr message::create_amqp_message()
    {
        f_timestamp = scarab::get_formatted_now();

        try
        {
            string t_body;
            encode_message_body( t_body );
            amqp_message_ptr t_message = AmqpClient::BasicMessage::Create( t_body );

            t_message->ContentEncoding( interpret_encoding() );
            t_message->CorrelationId( f_correlation_id );
            t_message->ReplyTo( f_reply_to );

            using AmqpClient::Table;
            using AmqpClient::TableEntry;
            using AmqpClient::TableValue;

            Table t_sender_info;
            t_sender_info.insert( TableEntry( "package", f_sender_package ) );
            t_sender_info.insert( TableEntry( "exe", f_sender_exe ) );
            t_sender_info.insert( TableEntry( "version", f_sender_version ) );
            t_sender_info.insert( TableEntry( "commit", f_sender_commit ) );
            t_sender_info.insert( TableEntry( "hostname", f_sender_hostname ) );
            t_sender_info.insert( TableEntry( "username", f_sender_username ) );
            t_sender_info.insert( TableEntry( "service_name", f_sender_service_name ) );

            Table t_properties;
            t_properties.insert( TableEntry( "msgtype", to_uint(message_type()) ) );
            t_properties.insert( TableEntry( "specifier", f_specifier.to_string() ) );
            t_properties.insert( TableEntry( "timestamp", f_timestamp ) );
            t_properties.insert( TableEntry( "sender_info", t_sender_info ) );

            this->derived_modify_amqp_message( t_message, t_properties );

            t_message->HeaderTable( t_properties );

            return t_message;
        }
        catch( dripline_error& e )
        {
            LERROR( dlog, e.what() );
            return amqp_message_ptr();
        }
    }

    void message::encode_message_body( std::string& a_body ) const
    {
        switch( f_encoding )
        {
            case encoding::json:
            {
                param_output_json t_output;
                if( ! t_output.write_string( *f_payload, a_body ) )
                {
                    throw dripline_error() << "Could not convert message body to string";
                }
                break;
            }
            default:
                throw dripline_error() << "Cannot encode using <" << interpret_encoding() << "> (" << f_encoding << ")";
                break;
        }
        return;
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

    scarab::param_node message::get_sender_info() const
    {
        scarab::param_node t_sender_info;
        t_sender_info.add( "package", f_sender_package );
        t_sender_info.add( "exe", f_sender_exe );
        t_sender_info.add( "version", f_sender_version );
        t_sender_info.add( "commit", f_sender_commit );
        t_sender_info.add( "hostname", f_sender_hostname );
        t_sender_info.add( "username", f_sender_username );
        t_sender_info.add( "service_name", f_sender_service_name );
        return t_sender_info;
    }

    void message::set_sender_info( const scarab::param_node& a_sender_info )
    {
        //f_sender_info = a_sender_info;
        //f_sender_info.add( "package", "N/A" ); // sets default if not present
        f_sender_package = a_sender_info["package"]().as_string();
        //f_sender_info.add( "exe", "N/A" ); // sets default if not present
        f_sender_exe = a_sender_info["exe"]().as_string();
        //f_sender_info.add( "version", "N/A" ); // sets default if not present
        f_sender_version = a_sender_info["version"]().as_string();
        //f_sender_info.add( "commit", "N/A" ); // sets default if not present
        f_sender_commit = a_sender_info["commit"]().as_string();
        //f_sender_info.add( "hostname", "N/A" ); // sets default if not present
        f_sender_hostname = a_sender_info["hostname"]().as_string();
        //f_sender_info.add( "username", "N/A" ); // sets default if not present
        f_sender_username = a_sender_info["username"]().as_string();
        //f_sender_info.add( "service_name", "N/A" ); // sets default if not present
        f_sender_service_name = a_sender_info["service_name"]().as_string();
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



    DRIPLINE_API bool operator==(  const message& a_lhs, const message& a_rhs )
    {
        return  a_lhs.routing_key() == a_rhs.routing_key() &&
                a_lhs.correlation_id() == a_rhs.correlation_id() &&
                a_lhs.reply_to() == a_rhs.reply_to() &&
                a_lhs.get_encoding() == a_rhs.get_encoding() &&
                a_lhs.timestamp() == a_rhs.timestamp() &&
                a_lhs.sender_commit() == a_rhs.sender_commit() &&
                a_lhs.sender_version() == a_rhs.sender_version() &&
                a_lhs.sender_package() == a_rhs.sender_package() &&
                a_lhs.sender_exe() == a_rhs.sender_exe() &&
                a_lhs.sender_hostname() == a_rhs.sender_hostname() &&
                a_lhs.sender_username() == a_rhs.sender_username() &&
                a_lhs.sender_service_name() == a_rhs.sender_service_name() &&
                a_lhs.parsed_specifier() == a_rhs.parsed_specifier() &&
                a_lhs.payload().to_string() == a_rhs.payload().to_string();
    }

    DRIPLINE_API bool operator==( const msg_request& a_lhs, const msg_request& a_rhs )
    {
        return operator==( static_cast< const message& >(a_lhs), static_cast< const message& >(a_rhs) ) &&
                a_lhs.lockout_key() == a_rhs.lockout_key() &&
                a_lhs.get_lockout_key_valid() == a_rhs.get_lockout_key_valid() &&
                a_lhs.get_message_op() == a_rhs.get_message_op();
    }

    DRIPLINE_API bool operator==( const msg_reply& a_lhs, const msg_reply& a_rhs )
    {
        return operator==( static_cast< const message& >(a_lhs), static_cast< const message& >(a_rhs) ) &&
                a_lhs.get_return_code() == a_rhs.get_return_code() &&
                a_lhs.return_msg() == a_rhs.return_msg();
    }

    DRIPLINE_API bool operator==( const msg_alert& a_lhs, const msg_alert& a_rhs )
    {
        return operator==( static_cast< const message& >(a_lhs), static_cast< const message& >(a_rhs) );
    }

    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, message::encoding a_enc )
    {
        static std::map< message::encoding, string > s_enc_strings = { { message::encoding::json, "json" } };
        return a_os << s_enc_strings[ a_enc ];
    }

    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, const message& a_message )
    {
        a_os << "Routing key: " << a_message.routing_key() << '\n';
        a_os << "Correlation ID: " << a_message.correlation_id() << '\n';
        a_os << "Reply To: " << a_message.reply_to() << '\n';
        a_os << "Encoding: " << a_message.get_encoding() << '\n';
        a_os << "Timestamp: " << a_message.timestamp() << '\n';
        a_os << "Sender Info:\n";
        a_os << "\tPackage: " << a_message.sender_package() << '\n';
        a_os << "\tExecutable: " << a_message.sender_exe() << '\n';
        a_os << "\tVersion: " << a_message.sender_version() << '\n';
        a_os << "\tCommit: " << a_message.sender_commit() << '\n';
        a_os << "\tHostname: " << a_message.sender_hostname() << '\n';
        a_os << "\tUsername: " << a_message.sender_username() << '\n';
        a_os << "\tService: " << a_message.sender_service_name() << '\n';
        a_os << "Specifier: " << a_message.parsed_specifier().unparsed() << '\n';
        return a_os;
    }

    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, const msg_request& a_message )
    {
        a_os << static_cast< const message& >( a_message );
        a_os << "Lockout Key: " << a_message.lockout_key() << '\n';
        a_os << "Lockout Key Valid: " << a_message.get_lockout_key_valid() << '\n';
        a_os << "Message Op: " << a_message.get_message_op() << '\n';
        return a_os;
    }

    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, const msg_reply& a_message )
    {
        a_os << static_cast< const message& >( a_message );
        a_os << "Return Code: " << a_message.get_return_code() << '\n';
        a_os << "Return Message: " << a_message.return_msg() << '\n';
        return a_os;
    }

    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, const msg_alert& a_message )
    {
        a_os << static_cast< const message& >( a_message );
        return a_os;
    }

} /* namespace dripline */
