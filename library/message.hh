/*
 * mt_message.hh
 *
 *  Created on: Jul 9, 2015
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_MESSAGE_HH_
#define DRIPLINE_MESSAGE_HH_

#include "member_variables.hh"
#include "param.hh"

#include "amqp.hh"
#include "dripline_api.hh"
#include "dripline_constants.hh"
#include "dripline_fwd.hh"
#include "return_codes.hh"
#include "specifier.hh"
#include "uuid.hh"

#include <memory>
#include <tuple>
#include <string>

namespace scarab
{
    class version_semantic;
}

namespace dripline
{
    class dripline_error;
    struct return_code;

    //***********
    // Message
    //***********

    /*!
     @class message
     @author N.S. Oblath

     @brief Contains all of the information common to all types of Dripline messages

     @details
     This is the base class for messages.  It contains all of the data members that are 
     common to the different Dripline message types.

     This class also contains the interface for converting between AMQP messages (Dripline 
     message chunks) and Dripline messages.
    */
    class DRIPLINE_API message
    {
        public:
            enum class encoding
            {
                json
            };

       public:
            message();
            message( const message& ) = delete;
            message( message&& ) = default;
            virtual ~message() = default;

            message& operator=( const message& ) = delete;
            message& operator=( message&& ) = default;

            virtual bool is_request() const = 0;
            virtual bool is_reply() const = 0;
            virtual bool is_alert() const = 0;

        public:
            /// Parses the message ID, which should be of the form [UUID]/[chunk]/[total chunks]
            static std::tuple< std::string, unsigned, unsigned > parse_message_id( const std::string& a_message_id );

            /// Converts a set of AMQP messages to a Dripline message object
            static message_ptr_t process_message( amqp_split_message_ptrs a_message_ptrs, const std::string& a_routing_key );

            /// Converts a Dripline message object to a set of AMQP messages
            amqp_split_message_ptrs create_amqp_messages( unsigned a_max_size = DL_MAX_PAYLOAD_SIZE );

            /// Converts the message-body to a strings (default encoding is JSON) for creating AMQP messages
            void encode_message_body( std::vector< std::string >& a_body_vec, unsigned a_max_size, const scarab::param_node& a_options = scarab::param_node() ) const;

            /// Converts the entire message into a single string (default encoding is JSON)
            std::string encode_full_message( unsigned a_max_size, const scarab::param_node& a_options = scarab::param_node() ) const;

        protected:
            virtual void derived_modify_amqp_message( amqp_message_ptr a_amqp_msg, AmqpClient::Table& a_properties ) const = 0;

            virtual void derived_modify_message_param( scarab::param_node& a_message_node ) const = 0;

            std::string interpret_encoding() const;

        public:
            /// Flag indicating whether the message was correctly converted from one or more AMQP messages
            mv_accessible( bool, is_valid );

            mv_referrable( std::string, routing_key );
            mv_referrable( std::string, correlation_id );
            mv_referrable( std::string, message_id );
            mv_referrable( std::string, reply_to );
            mv_accessible( encoding, encoding );
            mv_referrable( std::string, timestamp );

            mv_referrable( std::string, sender_exe );
            mv_referrable( std::string, sender_hostname );
            mv_referrable( std::string, sender_username );
            mv_referrable( std::string, sender_service_name );

            struct sender_package_version
            {
                std::string f_version;
                std::string f_commit;
                std::string f_package;
                sender_package_version();
                sender_package_version( const scarab::version_semantic& a_version );
                sender_package_version( const std::string& a_version, const std::string& a_commit, const std::string& a_package );
                bool operator==( const sender_package_version& a_rhs ) const;
            };
            typedef std::map< std::string, sender_package_version > sender_version_map_t;
            mv_referrable( sender_version_map_t, sender_versions );

        protected:
            mutable specifier f_specifier;

        public:
            specifier& parsed_specifier();
            const specifier& parsed_specifier() const;

            virtual msg_t message_type() const = 0;

            /// Creates and returns a new param_node object to contain the sender info
            scarab::param_node get_sender_info() const;
            /// Copies the sender info out of a param_node
            void set_sender_info( const scarab::param_node& a_sender_info );

            /// Creates and returns a new param_node object to contain the full message
            scarab::param_node get_message_param( bool a_include_payload = true ) const;

        public:
            scarab::param& payload();
            const scarab::param& payload() const;

            void set_payload( scarab::param_ptr_t a_payload );
            const scarab::param_ptr_t& get_payload_ptr() const;

        private:
            scarab::param_ptr_t f_payload;

        public:
            static const char s_message_id_separator = '/';

    };

    DRIPLINE_API bool operator==( const message& a_lhs, const message& a_rhs );

    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, message::encoding a_enc );

    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, const message& a_message );

    //***********
    // Request
    //***********

    /*!
     @class msg_request
     @author N.S. Oblath

     @brief Request message class

     @details
     Adds the lockout information and the message operation.

     Requests can be created using the static function `create()`.
    */
    class DRIPLINE_API msg_request : public message
    {
        public:
            msg_request();
            msg_request( const msg_request& ) = delete;
            msg_request( msg_request&& ) = default;
            virtual ~msg_request() = default;

            msg_request& operator=( const msg_request& ) = delete;
            msg_request& operator=( msg_request&& ) = default;

            /// Create a request message
            static request_ptr_t create( scarab::param_ptr_t a_payload, op_t a_msg_op, const std::string& a_routing_key, const std::string& a_specifier = "", const std::string& a_reply_to = "", message::encoding a_encoding = encoding::json );

            bool is_request() const;
            bool is_reply() const;
            bool is_alert() const;

            /// Creates a reply message using the reply-to and correlation ID information in this message
            reply_ptr_t reply( const return_code& a_return_code, const std::string& a_ret_msg, scarab::param_ptr_t a_payload = scarab::param_ptr_t( new scarab::param() ) ) const;
            /// Creates a reply message using the reply-to and correlation ID information in this message
            reply_ptr_t reply( const unsigned a_return_code, const std::string& a_ret_msg, scarab::param_ptr_t a_payload = scarab::param_ptr_t( new scarab::param() ) ) const;

        private:
            void derived_modify_amqp_message( amqp_message_ptr a_amqp_msg, AmqpClient::Table& a_properties ) const;
            virtual void derived_modify_message_param( scarab::param_node& a_message_node ) const;

        public:
            virtual msg_t message_type() const;
            mv_accessible_static_noset( msg_t, message_type );

            mv_referrable( uuid_t, lockout_key );
            mv_accessible( bool, lockout_key_valid );
            mv_accessible( op_t, message_operation );

    };

    DRIPLINE_API bool operator==( const msg_request& a_lhs, const msg_request& a_rhs );

    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, const msg_request& a_message );


    //*********
    // Reply
    //*********

    /*!
     @class msg_reply
     @author N.S. Oblath

     @brief Reply message class

     @details
     Adds the return code and return message.

     Reply messages can be created with the `create()` functions.  A variety of `create()` function 
     overloads are available, depending on whether the return code is specified with a `return_code` 
     object or as an int, and depending on whether a request message is used to fill out some of the 
     header information or if it's done manually.
    */
    class DRIPLINE_API msg_reply : public message
    {
        public:
            msg_reply();
            msg_reply( const msg_reply& ) = delete;
            msg_reply( msg_reply&& ) = default;
            virtual ~msg_reply() = default;

            msg_reply& operator=( const msg_reply& ) = delete;
            msg_reply& operator=( msg_reply&& ) = default;

            /// Create a reply message using a return_code object and manually specifying the destination
            static reply_ptr_t create( const return_code& a_return_code, const std::string& a_ret_msg, scarab::param_ptr_t a_payload, const std::string& a_routing_key, const std::string& a_specifier = "", message::encoding a_encoding = encoding::json );
            /// Create a reply message using an integer return code and manually specifying the destination
            static reply_ptr_t create( unsigned a_return_code_value, const std::string& a_ret_msg, scarab::param_ptr_t a_payload, const std::string& a_routing_key, const std::string& a_specifier = "", message::encoding a_encoding = encoding::json );
            /// Create a reply message using a return_code object and a request message for the destination and correlation ID
            static reply_ptr_t create( const return_code& a_return_code, const std::string& a_ret_msg, scarab::param_ptr_t a_payload, const msg_request& a_request );
            /// Create a reply message using an integer return code and a request message for the destination and correlation ID
            static reply_ptr_t create( unsigned a_return_code, const std::string& a_ret_msg, scarab::param_ptr_t a_payload, const msg_request& a_request );

            bool is_request() const;
            bool is_reply() const;
            bool is_alert() const;

        private:
            void derived_modify_amqp_message( amqp_message_ptr a_amqp_msg, AmqpClient::Table& a_properties ) const;
            virtual void derived_modify_message_param( scarab::param_node& a_message_node ) const;

        public:
            virtual msg_t message_type() const;
            mv_accessible_static_noset( msg_t, message_type );

            mv_accessible( unsigned, return_code );
            mv_referrable( std::string, return_message );

        private:
            mutable std::string f_return_buffer;

    };

    DRIPLINE_API bool operator==( const msg_reply& a_lhs, const msg_reply& a_rhs );

    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, const msg_reply& a_message );


    //*********
    // Alert
    //*********

    /*!
     @class msg_alert
     @author N.S. Oblath

     @brief Alert message class

     @details
     No additional fields are added.

     Alert message can be created with the static `create()` function.
    */
    class DRIPLINE_API msg_alert : public message
    {
        public:
            msg_alert();
            msg_alert( const msg_alert& ) = delete;
            msg_alert( msg_alert&& ) = default;
            virtual ~msg_alert() = default;

            msg_alert& operator=( const msg_alert& ) = delete;
            msg_alert& operator=( msg_alert&& ) = default;

            /// Creates an alert message
            static alert_ptr_t create( scarab::param_ptr_t a_payload, const std::string& a_routing_key, const std::string& a_specifier = "", message::encoding a_encoding = encoding::json );

            bool is_request() const;
            bool is_reply() const;
            bool is_alert() const;

        private:
            void derived_modify_amqp_message( amqp_message_ptr a_amqp_msg, AmqpClient::Table& a_properties ) const;
            virtual void derived_modify_message_param( scarab::param_node& a_message_node ) const;

        public:
            virtual msg_t message_type() const;
            mv_accessible_static_noset( msg_t, message_type );

    };

    DRIPLINE_API bool operator==( const msg_alert& a_lhs, const msg_alert& a_rhs );

    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, const msg_alert& a_message );


    //***********
    // Message
    //***********

    inline specifier& message::parsed_specifier()
    {
        return f_specifier;
    }

    inline const specifier& message::parsed_specifier() const
    {
        return f_specifier;
    }

    inline scarab::param& message::payload()
    {
        return *f_payload;
    }

    inline const scarab::param& message::payload() const
    {
        return *f_payload;
    }

    inline void message::set_payload( scarab::param_ptr_t a_payload )
    {
        f_payload = std::move(a_payload);
    }

    inline const scarab::param_ptr_t& message::get_payload_ptr() const
    {
        return f_payload;
    }


    //***********
    // Request
    //***********

    inline bool msg_request::is_request() const
    {
        return true;
    }
    inline bool msg_request::is_reply() const
    {
        return false;
    }
    inline bool msg_request::is_alert() const
    {
        return false;
    }

    inline void msg_request::derived_modify_amqp_message( amqp_message_ptr /*a_amqp_msg*/, AmqpClient::Table& a_properties ) const
    {
        a_properties.insert( AmqpClient::TableEntry( "message_operation", AmqpClient::TableValue(to_int(f_message_operation)) ) );
        a_properties.insert( AmqpClient::TableEntry( "lockout_key", AmqpClient::TableValue(string_from_uuid(lockout_key())) ) );
        return;
    }

    inline void msg_request::derived_modify_message_param( scarab::param_node& a_message_node ) const
    {
        a_message_node.add( "message_operation", to_int(f_message_operation) );
        a_message_node.add( "lockout_key", string_from_uuid(lockout_key()) );
        return;
    }

    inline reply_ptr_t msg_request::reply( const return_code& a_return_code, const std::string& a_ret_msg, scarab::param_ptr_t a_payload ) const
    {
        return msg_reply::create( a_return_code, a_ret_msg, std::move(a_payload), *this );
    }

    inline reply_ptr_t msg_request::reply( unsigned a_return_code, const std::string& a_ret_msg, scarab::param_ptr_t a_payload ) const
    {
        return msg_reply::create( a_return_code, a_ret_msg, std::move(a_payload), *this );
    }


    //*********
    // Reply
    //*********

    inline reply_ptr_t msg_reply::create( const return_code& a_return_code, const std::string& a_ret_msg, scarab::param_ptr_t a_payload, const msg_request& a_request )
    {
        return msg_reply::create( a_return_code.rc_value(), a_ret_msg, std::move(a_payload), a_request );
    }

    inline reply_ptr_t msg_reply::create( unsigned a_return_code, const std::string& a_ret_msg, scarab::param_ptr_t a_payload, const msg_request& a_request )
    {
        reply_ptr_t t_reply = msg_reply::create( a_return_code, a_ret_msg, std::move(a_payload), a_request.reply_to(), "", a_request.get_encoding() );
        t_reply->correlation_id() = a_request.correlation_id();
        return t_reply;
    }

    inline bool msg_reply::is_request() const
    {
        return false;
    }
    inline bool msg_reply::is_reply() const
    {
        return true;
    }
    inline bool msg_reply::is_alert() const
    {
        return false;
    }

    inline void msg_reply::derived_modify_amqp_message( amqp_message_ptr, AmqpClient::Table& a_properties ) const
    {
        a_properties.insert( AmqpClient::TableEntry( "return_code", AmqpClient::TableValue(f_return_code) ) );
        a_properties.insert( AmqpClient::TableEntry( "return_message", AmqpClient::TableValue(f_return_message) ) );
        return;
    }

    inline void msg_reply::derived_modify_message_param( scarab::param_node& a_message_node ) const
    {
        a_message_node.add( "return_code", f_return_code );
        a_message_node.add( "return_message", f_return_message );
        return;
    }


    //*********
    // Alert
    //*********

    inline bool msg_alert::is_request() const
    {
        return false;
    }
    inline bool msg_alert::is_reply() const
    {
        return false;
    }
    inline bool msg_alert::is_alert() const
    {
        return true;
    }

    inline void msg_alert::derived_modify_amqp_message( amqp_message_ptr, AmqpClient::Table& ) const
    {
        return;
    }

    inline void msg_alert::derived_modify_message_param( scarab::param_node& ) const
    {
        return;
    }

} /* namespace dripline */

#endif /* DRIPLINE_MESSAGE_HH_ */
