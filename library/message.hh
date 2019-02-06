/*
 * mt_message.hh
 *
 *  Created on: Jul 9, 2015
 *      Author: nsoblath
 */

#ifndef DRIPLINE_MESSAGE_HH_
#define DRIPLINE_MESSAGE_HH_

#include "member_variables.hh"
#include "param.hh"

#include "amqp.hh"
#include "dripline_api.hh"
#include "dripline_constants.hh"
#include "specifier.hh"
#include "uuid.hh"

#include <memory>
#include <tuple>
#include <string>

namespace dripline
{
    class dripline_error;
    struct return_code;

    class message;
    class msg_request;
    class msg_reply;
    class msg_alert;

    typedef std::shared_ptr< message > message_ptr_t;
    typedef std::shared_ptr< msg_request > request_ptr_t;
    typedef std::shared_ptr< msg_reply > reply_ptr_t;
    typedef std::shared_ptr< msg_alert > alert_ptr_t;

    //***********
    // Message
    //***********

    class DRIPLINE_API message
    {
        public:
            enum class encoding
            {
                json
            };

       public:
            message();
            virtual ~message();

            virtual bool is_request() const = 0;
            virtual bool is_reply() const = 0;
            virtual bool is_alert() const = 0;

        public:
            /// Parses the message ID, which should be of the form [UUID]/[chunk]/[total chunks]
            static std::tuple< std::string, unsigned, unsigned > parse_message_id( const std::string& a_message_id );

            /// from AMQP to message object
            //static message_ptr_t process_envelope( amqp_envelope_ptr a_envelope );
            static message_ptr_t process_message( amqp_split_message_ptrs a_message_ptrs, const std::string& a_routing_key );

            /// from message object to AMQP
            amqp_split_message_ptrs create_amqp_messages( unsigned a_max_size = 1000 );

            /// from message object to string
            void encode_message_body( std::vector< std::string >& a_body_vec, unsigned a_max_size ) const;

        protected:
            virtual void derived_modify_amqp_message( amqp_message_ptr t_amqp_msg, AmqpClient::Table& t_properties ) const = 0;

            std::string interpret_encoding() const;

        public:
            mv_accessible( bool, is_valid );

            mv_referrable( std::string, routing_key );
            mv_referrable( std::string, correlation_id );
            mv_referrable( std::string, message_id );
            mv_referrable( std::string, reply_to );
            mv_accessible( encoding, encoding );
            mv_referrable( std::string, timestamp );

            mv_referrable( std::string, sender_package );
            mv_referrable( std::string, sender_exe );
            mv_referrable( std::string, sender_version );
            mv_referrable( std::string, sender_commit );
            mv_referrable( std::string, sender_hostname );
            mv_referrable( std::string, sender_username );
            mv_referrable( std::string, sender_service_name );

        protected:
            mutable specifier f_specifier;

        public:
            specifier& parsed_specifier();
            const specifier& parsed_specifier() const;

            virtual msg_t message_type() const = 0;

            //void set_sender_package( const std::string& a_pkg );
            //void set_sender_exe( const std::string& a_exe );
            //void set_sender_version( const std::string& a_vsn );
            //void set_sender_commit( const std::string& a_cmt );
            //void set_sender_hostname( const std::string& a_host );
            //void set_sender_username( const std::string& a_user );
            //void set_sender_service_name( const std::string& a_service );

            scarab::param_node get_sender_info() const;
            void set_sender_info( const scarab::param_node& a_sender_info );

        public:
            scarab::param& payload();
            const scarab::param& payload() const;

            void set_payload( scarab::param_ptr_t a_payload );

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

    class DRIPLINE_API msg_request : public message
    {
        public:
            msg_request();
            virtual ~msg_request();

            static request_ptr_t create( scarab::param_ptr_t a_payload, op_t a_msg_op, const std::string& a_routing_key, const std::string& a_specifier = "", const std::string& a_reply_to = "", message::encoding a_encoding = encoding::json );

            bool is_request() const;
            bool is_reply() const;
            bool is_alert() const;

            reply_ptr_t reply(  const return_code& a_retcode, const std::string& a_ret_msg, scarab::param_ptr_t a_payload = scarab::param_ptr_t( new scarab::param() ) ) const;

        private:
            void derived_modify_amqp_message( amqp_message_ptr t_amqp_msg, AmqpClient::Table& t_properties ) const;

        public:
            virtual msg_t message_type() const;
            mv_accessible_static_noset( msg_t, message_type );

            mv_referrable( uuid_t, lockout_key );
            mv_accessible( bool, lockout_key_valid );
            mv_accessible( op_t, message_op );

    };

    DRIPLINE_API bool operator==( const msg_request& a_lhs, const msg_request& a_rhs );

    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, const msg_request& a_message );


    //*********
    // Reply
    //*********

    class DRIPLINE_API msg_reply : public message
    {
        public:
            msg_reply();
            virtual ~msg_reply();

            static reply_ptr_t create( const return_code& a_retcode, const std::string& a_ret_msg, scarab::param_ptr_t a_payload, const std::string& a_routing_key, const std::string& a_specifier = "", message::encoding a_encoding = encoding::json );
            static reply_ptr_t create( const return_code& a_retcode, const std::string& a_ret_msg, scarab::param_ptr_t a_payload, const msg_request& a_request );
            static reply_ptr_t create( unsigned a_retcode_value, const std::string& a_ret_msg, scarab::param_ptr_t a_payload, const std::string& a_routing_key, const std::string& a_specifier = "", message::encoding a_encoding = encoding::json );

            bool is_request() const;
            bool is_reply() const;
            bool is_alert() const;

        private:
            void derived_modify_amqp_message( amqp_message_ptr t_amqp_msg, AmqpClient::Table& t_properties ) const;

        public:
            virtual msg_t message_type() const;
            mv_accessible_static_noset( msg_t, message_type );

            mv_accessible( unsigned, return_code );
            mv_referrable( std::string, return_msg );

        private:
            mutable std::string f_return_buffer;

    };

    DRIPLINE_API bool operator==( const msg_reply& a_lhs, const msg_reply& a_rhs );

    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, const msg_reply& a_message );


    //*********
    // Alert
    //*********

    class DRIPLINE_API msg_alert : public message
    {
        public:
            msg_alert();
            virtual ~msg_alert();

            static alert_ptr_t create( scarab::param_ptr_t a_payload, const std::string& a_routing_key, const std::string& a_specifier = "", message::encoding a_encoding = encoding::json );

            bool is_request() const;
            bool is_reply() const;
            bool is_alert() const;

        private:
            void derived_modify_amqp_message( amqp_message_ptr t_amqp_msg, AmqpClient::Table& t_properties ) const;

        public:
            virtual msg_t message_type() const;
            mv_accessible_static_noset( msg_t, message_type );

    };

    DRIPLINE_API bool operator==( const msg_alert& a_lhs, const msg_alert& a_rhs );

    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, const msg_alert& a_message );


    //***********
    // Message
    //***********

/*
    inline void message::set_sender_package( const std::string& a_pkg )
    {
        //f_sender_info["package"]().set( a_pkg );
        f_sender_package = a_pkg;
        return;
    }

    inline void message::set_sender_exe( const std::string& a_exe )
    {
        //f_sender_info["exe"]().set( a_exe );
        f_sender_exe = a_exe;
        return;
    }

    inline void message::set_sender_version( const std::string& a_vsn )
    {
        //f_sender_info["version"]().set( a_vsn );
        f_sender_version = a_vsn;
        return;
    }

    inline void message::set_sender_commit( const std::string& a_cmt )
    {
        //f_sender_info["commit"]().set( a_cmt );
        f_sender_commit = a_cmt;
        return;
    }

    inline void message::set_sender_hostname( const std::string& a_host )
    {
        //f_sender_info["hostname"]().set( a_host );
        f_sender_hostname = a_host;
        return;
    }

    inline void message::set_sender_username( const std::string& a_user )
    {
        //f_sender_info["username"]().set( a_user );
        f_sender_username = a_user;
        return;
    }

    inline void message::set_sender_service_name( const std::string& a_service )
    {
        //f_sender_info["service_name"]().set( a_service );
        f_sender_service_name = a_service;
        return;
    }
*/
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
        a_properties.insert( AmqpClient::TableEntry( "msgop", AmqpClient::TableValue(to_uint(f_message_op)) ) );
        a_properties.insert( AmqpClient::TableEntry( "lockout_key", AmqpClient::TableValue(string_from_uuid(lockout_key())) ) );
        return;
    }

    inline reply_ptr_t msg_request::reply( const return_code& a_retcode, const std::string& a_ret_msg, scarab::param_ptr_t a_payload ) const
    {
        return msg_reply::create( a_retcode, a_ret_msg, std::move(a_payload), *this );
    }


    //*********
    // Reply
    //*********

    inline reply_ptr_t msg_reply::create( const return_code& a_retcode, const std::string& a_ret_msg, scarab::param_ptr_t a_payload, const msg_request& a_request )
    {
        reply_ptr_t t_reply = msg_reply::create( a_retcode, a_ret_msg, std::move(a_payload), a_request.reply_to(), "", a_request.get_encoding() );
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

    inline void msg_reply::derived_modify_amqp_message( amqp_message_ptr /*a_amqp_msg*/, AmqpClient::Table& a_properties ) const
    {
        a_properties.insert( AmqpClient::TableEntry( "retcode", AmqpClient::TableValue(f_return_code) ) );
        a_properties.insert( AmqpClient::TableEntry( "return_msg", AmqpClient::TableValue(f_return_msg) ) );
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

    inline void msg_alert::derived_modify_amqp_message( amqp_message_ptr /*a_amqp_msg*/, AmqpClient::Table& /*a_properties*/ ) const
    {
        return;
    }

} /* namespace dripline */

#endif /* DRIPLINE_MESSAGE_HH_ */
