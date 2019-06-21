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

#include "logger.hh"
#include "map_at_default.hh"
#include "param_json.hh"
#include "return_codes.hh"
#include "time.hh"
#include "version_wrapper.hh"

#include <cmath>
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
            f_is_valid( true ),
            f_routing_key(),
            f_correlation_id(),
            f_message_id(),
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
/*
    message_ptr_t message::process_envelope( amqp_envelope_ptr a_envelope )
    {
        if( ! a_envelope )
        {
            throw dripline_error() << "Empty envelope received";
        }
        return message::process_message( a_envelope->Message(), a_envelope->RoutingKey() );
    }
*/
    std::tuple< std::string, unsigned, unsigned > message::parse_message_id( const string& a_message_id )
    {
        std::string::size_type t_first_separator = a_message_id.find_first_of( s_message_id_separator );
        std::string::size_type t_last_separator = a_message_id.find_last_of( s_message_id_separator );
        if( t_first_separator == a_message_id.npos || t_last_separator == a_message_id.npos )
        {
            throw dripline_error() << "Message ID is not formatted correctly\nShould be [UUID]/[chunk]/[total chunks]\nReceived: " << a_message_id;
        }

        return std::make_tuple( a_message_id.substr(0, t_first_separator),
                                std::stoul(a_message_id.substr(t_first_separator + 1, t_last_separator - t_first_separator - 1)),
                                std::stoul(a_message_id.substr(t_last_separator + 1)) );
    }

    message_ptr_t message::process_message( amqp_split_message_ptrs a_message_ptrs, const std::string& a_routing_key )
    {
        // find first non-empty message pointer
        // get length of payload (to use for empty ones)
        // get header content
        // set bool for complete to true
        // loop through messages to build full payload string, if one is incomplete, fill  with hashes and set bool for complete to false
        // if payload complete, parse payload

        if( a_message_ptrs.empty() )
        {
            throw dripline_error() << "No messages were provided for processing";
        }

        // Find the first valid message, from which we'll get the payload chunk length
        // Below we'll also use this for the header content
        amqp_message_ptr t_first_valid_message;
        for( unsigned i_message = 0; ! t_first_valid_message && i_message < a_message_ptrs.size(); ++i_message )
        {
            t_first_valid_message = a_message_ptrs[i_message];
        }

        if( ! t_first_valid_message )
        {
            throw dripline_error() << "All messages provided for processing were invalid";
        }

        unsigned t_payload_chunk_length = t_first_valid_message->Body().size();

        encoding t_encoding;
        if( t_first_valid_message->ContentEncoding() == "application/json" )
        {
            t_encoding = encoding::json;
        }
        else
        {
            throw dripline_error() << "Unable to parse message with content type <" << t_first_valid_message->ContentEncoding() << ">";
        }

        // Build up the body
        string t_payload_str;
        bool t_payload_is_complete = true;
        for( amqp_message_ptr& t_message : a_message_ptrs )
        {
            if( ! t_message )
            {
                // If a chunk of the message is missing, it's filled with hashes
                t_payload_is_complete = false;
                t_payload_str += string( t_payload_chunk_length, '#' );
                continue;
            }

            t_payload_str += t_message->Body();
        }

        // Attempt to parse
        scarab::param_ptr_t t_payload;
        string t_payload_error_msg;
        if( t_payload_is_complete )
        {
            // Parse the body
            param_input_json t_input;
            t_payload = t_input.read_string( t_payload_str );
            if( ! t_payload )
            {
                t_payload_error_msg = "Message body could not be parsed; skipping request";
            }
            if( ! t_payload->is_node() )
            {
                t_payload_error_msg = "Payload did not parse into a node";
            }
        }
        else
        {
            t_payload_error_msg = "Entire message was not available";
        }

        // Payload is unavailable if the error message is non-empty
        // In that case, store whatever we have for the payload string in the payload, plus the error message
        if( ! t_payload_error_msg.empty() )
        {
            // Store the invalid payload string in the payload
            t_payload = std::unique_ptr< scarab::param_node >( new param_node() );
            t_payload->as_node().add( "invalid", t_payload_str );
            t_payload->as_node().add( "error", t_payload_error_msg );
        }

        LDEBUG( dlog, "Processing message:\n" <<
                 "Routing key: " << a_routing_key << '\n' <<
                 "Payload: " << t_payload_str );

        using scarab::at;

        using AmqpClient::Table;
        using AmqpClient::TableEntry;
        using AmqpClient::TableValue;
        Table t_properties = t_first_valid_message->HeaderTable();

        // Create the message, of whichever type
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
                        t_first_valid_message->ReplyTo(),
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

        // Set message header fields
        if( ! t_payload_error_msg.empty() )
        {
            t_message->set_is_valid( false );
        }

        t_message->correlation_id() = t_first_valid_message->CorrelationId();
        t_message->message_id() = t_first_valid_message->MessageId();
        // remove the message chunk information from the message id
        t_message->message_id() = t_message->message_id().substr( 0, t_message->message_id().find_first_of(s_message_id_separator) );
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

    amqp_split_message_ptrs message::create_amqp_messages( unsigned a_max_size )
    {
        f_timestamp = scarab::get_formatted_now();

        try
        {
            std::vector< string > t_body_parts;
            encode_message_body( t_body_parts, a_max_size );

            unsigned t_n_chunks = t_body_parts.size();
            std::vector< amqp_message_ptr > t_message_parts( t_n_chunks );

            if( f_message_id.empty() )
            {
                f_message_id = string_from_uuid( generate_random_uuid() );
            }
            string t_base_message_id = f_message_id + s_message_id_separator;
            string t_total_chunks_str = s_message_id_separator + std::to_string(t_n_chunks);

            unsigned i_chunk = 0;
            for( string& t_body_part : t_body_parts )
            {
                amqp_message_ptr t_message = AmqpClient::BasicMessage::Create( t_body_part );

                t_message->ContentEncoding( interpret_encoding() );
                t_message->CorrelationId( f_correlation_id );
                t_message->MessageId( t_base_message_id + std::to_string(i_chunk) + t_total_chunks_str );
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

                t_message_parts[i_chunk] = t_message;

                ++i_chunk;
            }

            return t_message_parts;
        }
        catch( dripline_error& e )
        {
            LERROR( dlog, e.what() );
            return std::vector< amqp_message_ptr >();
        }
    }

    void message::encode_message_body( std::vector< string >& a_body_vec, unsigned a_max_size, const scarab::param_node& a_options ) const
    {
        switch( f_encoding )
        {
            case encoding::json:
            {
                string t_body;
                param_output_json t_output;
                if( ! t_output.write_string( *f_payload, t_body, a_options ) )
                {
                    throw dripline_error() << "Could not convert message body to string";
                }

                unsigned t_chars_per_chunk = a_max_size / sizeof(string::value_type);
                unsigned t_n_chunks = std::ceil( double(t_body.size()) / double(t_chars_per_chunk) );
                a_body_vec.resize( t_n_chunks );
                for( unsigned i_chunk = 0, pos = 0; pos < t_body.size(); pos += t_chars_per_chunk, ++i_chunk )
                {
                    a_body_vec[i_chunk] = t_body.substr(pos, t_chars_per_chunk );
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
        if( a_message.payload().is_node() ) a_os << "Payload: " << a_message.payload().as_node() << '\n';
        else if( a_message.payload().is_array() ) a_os << "Payload: " << a_message.payload().as_array() << '\n';
        else if( a_message.payload().is_value() ) a_os << "Payload: " << a_message.payload().as_value() << '\n';
        else a_os << "Payload: null\n";
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
