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
#include <string>

namespace dripline
{
    class dripline_error;

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
            /// from AMQP to message object
            static message_ptr_t process_envelope( amqp_envelope_ptr a_envelope );

            /// from message object to AMQP
            amqp_message_ptr create_amqp_message() const;

            /// from message object to string
            bool encode_message_body( std::string& a_body ) const;

        protected:
            virtual bool derived_modify_amqp_message( amqp_message_ptr t_amqp_msg ) const = 0;
            virtual bool derived_modify_message_body( scarab::param_node& a_node ) const = 0;

            std::string interpret_encoding() const;

        public:
            mv_referrable( std::string, routing_key );
            mv_referrable( std::string, correlation_id );
            mv_referrable( std::string, reply_to );
            mv_accessible( encoding, encoding );
            mv_referrable( std::string, timestamp );

            mv_referrable_const( std::string, sender_package );
            mv_referrable_const( std::string, sender_exe );
            mv_referrable_const( std::string, sender_version );
            mv_referrable_const( std::string, sender_commit );
            mv_referrable_const( std::string, sender_hostname );
            mv_referrable_const( std::string, sender_username );
            mv_referrable_const( std::string, sender_service_name );

            mv_referrable_const( scarab::param_node, sender_info );

        protected:
            mutable specifier f_specifier;

        public:
            specifier& parsed_specifier();
            const specifier& parsed_specifier() const;

            virtual msg_t message_type() const = 0;

            void set_sender_package( const std::string& a_pkg );
            void set_sender_exe( const std::string& a_exe );
            void set_sender_version( const std::string& a_vsn );
            void set_sender_commit( const std::string& a_cmt );
            void set_sender_hostname( const std::string& a_host );
            void set_sender_username( const std::string& a_user );
            void set_sender_service_name( const std::string& a_service );

            void set_sender_info( const scarab::param_node& a_payload );

        public:
            scarab::param& payload();
            const scarab::param& payload() const;

        private:
            scarab::param_ptr_t f_payload;

    };

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

            static request_ptr_t create( const scarab::param& a_payload, op_t a_msg_op, const std::string& a_routing_key, const std::string& a_specifier = "", const std::string& a_reply_to = "", message::encoding a_encoding = encoding::json );

            bool is_request() const;
            bool is_reply() const;
            bool is_alert() const;

            template< typename x_retcode >
            reply_ptr_t reply( const std::string& a_ret_msg, const scarab::param& a_payload = scarab::param() ) const;

        private:
            bool derived_modify_amqp_message( amqp_message_ptr t_amqp_msg ) const;
            bool derived_modify_message_body( scarab::param_node& a_node ) const;

        public:
            virtual msg_t message_type() const;
            mv_accessible_static_noset( msg_t, message_type );

            mv_referrable( uuid_t, lockout_key );
            mv_accessible( bool, lockout_key_valid );
            mv_accessible( op_t, message_op );

    };


    //*********
    // Reply
    //*********

    class DRIPLINE_API msg_reply : public message
    {
        public:
            msg_reply();
            virtual ~msg_reply();

            template< typename x_retcode >
            static reply_ptr_t create( const std::string& a_ret_msg, const scarab::param& a_payload, const std::string& a_routing_key, const std::string& a_specifier = "", message::encoding a_encoding = encoding::json );
            template< typename x_retcode >
            static reply_ptr_t create( const std::string& a_ret_msg, const scarab::param& a_payload, const msg_request& a_request );
            static reply_ptr_t create( unsigned a_retcode, const std::string& a_ret_msg, const scarab::param& a_payload, const std::string& a_routing_key, const std::string& a_specifier = "", message::encoding a_encoding = encoding::json );
            //static reply_ptr_t create( const reply_package_2& a_reply_package, const std::string& a_routing_key, const std::string& a_specifier = "", message::encoding a_encoding = encoding::json );
            static reply_ptr_t create( const dripline_error& a_error, const std::string& a_routing_key, const std::string& a_specifier = "", message::encoding a_encoding = encoding::json );

            bool is_request() const;
            bool is_reply() const;
            bool is_alert() const;

        private:
            bool derived_modify_amqp_message( amqp_message_ptr t_amqp_msg ) const;
            bool derived_modify_message_body( scarab::param_node& a_node ) const;

        public:
            virtual msg_t message_type() const;
            mv_accessible_static_noset( msg_t, message_type );

            mv_accessible( unsigned, return_code );
            mv_referrable( std::string, return_msg );

        private:
            mutable std::string f_return_buffer;

    };

    //*********
    // Alert
    //*********

    class DRIPLINE_API msg_alert : public message
    {
        public:
            msg_alert();
            virtual ~msg_alert();

            static alert_ptr_t create( const scarab::param&, const std::string& a_routing_key, const std::string& a_specifier = "", message::encoding a_encoding = encoding::json );

            bool is_request() const;
            bool is_reply() const;
            bool is_alert() const;

        private:
            bool derived_modify_amqp_message( amqp_message_ptr t_amqp_msg ) const;
            bool derived_modify_message_body( scarab::param_node& a_node ) const;

        public:
            virtual msg_t message_type() const;
            mv_accessible_static_noset( msg_t, message_type );

    };


    //***********
    // Message
    //***********

    inline void message::set_sender_info( const scarab::param_node& a_sender_info )
    {
        f_sender_info = a_sender_info;
        f_sender_info.add( "package", "N/A" ); // sets default if not present
        f_sender_package = f_sender_info["package"]().as_string();
        f_sender_info.add( "exe", "N/A" ); // sets default if not present
        f_sender_exe = f_sender_info["exe"]().as_string();
        f_sender_info.add( "version", "N/A" ); // sets default if not present
        f_sender_version = f_sender_info["version"]().as_string();
        f_sender_info.add( "commit", "N/A" ); // sets default if not present
        f_sender_commit = f_sender_info["commit"]().as_string();
        f_sender_info.add( "hostname", "N/A" ); // sets default if not present
        f_sender_hostname = f_sender_info["hostname"]().as_string();
        f_sender_info.add( "username", "N/A" ); // sets default if not present
        f_sender_username = f_sender_info["username"]().as_string();
        f_sender_info.add( "service_name", "N/A" ); // sets default if not present
        f_sender_service_name = f_sender_info["service_name"]().as_string();
    }

    inline void message::set_sender_package( const std::string& a_pkg )
    {
        f_sender_info["package"]().set( a_pkg );
        f_sender_package = a_pkg;
        return;
    }

    inline void message::set_sender_exe( const std::string& a_exe )
    {
        f_sender_info["exe"]().set( a_exe );
        f_sender_exe = a_exe;
        return;
    }

    inline void message::set_sender_version( const std::string& a_vsn )
    {
        f_sender_info["version"]().set( a_vsn );
        f_sender_version = a_vsn;
        return;
    }

    inline void message::set_sender_commit( const std::string& a_cmt )
    {
        f_sender_info["commit"]().set( a_cmt );
        f_sender_commit = a_cmt;
        return;
    }

    inline void message::set_sender_hostname( const std::string& a_host )
    {
        f_sender_info["hostname"]().set( a_host );
        f_sender_hostname = a_host;
        return;
    }

    inline void message::set_sender_username( const std::string& a_user )
    {
        f_sender_info["username"]().set( a_user );
        f_sender_username = a_user;
        return;
    }

    inline void message::set_sender_service_name( const std::string& a_service )
    {
        f_sender_info["service_name"]().set( a_service );
        f_sender_service_name = a_service;
        return;
    }

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

    inline bool msg_request::derived_modify_amqp_message( amqp_message_ptr /*a_amqp_msg*/ ) const
    {
        return true;
    }

    inline bool msg_request::derived_modify_message_body( scarab::param_node& a_node ) const
    {
        a_node.add( "msgop", to_uint(f_message_op) );
        a_node.add( "lockout_key", string_from_uuid( lockout_key() ) );
        return true;
    }

    template< typename x_retcode >
    inline reply_ptr_t msg_request::reply( const std::string& a_ret_msg, const scarab::param& a_payload ) const
    {
        return msg_reply::create< x_retcode >( a_ret_msg, a_payload, *this );
    }


    //*********
    // Reply
    //*********

    template< typename x_retcode >
    reply_ptr_t msg_reply::create( const std::string& a_ret_msg, const scarab::param& a_payload, const std::string& a_routing_key, const std::string& a_specifier, message::encoding a_encoding )
    {
        return msg_reply::create( x_retcode::f_code, a_ret_msg, a_payload, a_routing_key, a_specifier, a_encoding );
    }

    template< typename x_retcode >
    reply_ptr_t msg_reply::create( const std::string& a_ret_msg, const scarab::param& a_payload, const msg_request& a_request )
    {
        reply_ptr_t t_reply = msg_reply::create( x_retcode::f_code, a_ret_msg, a_payload, a_request.reply_to(), "", a_request.get_encoding() );
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

    inline bool msg_reply::derived_modify_amqp_message( amqp_message_ptr /*a_amqp_msg*/ ) const
    {
        return true;
    }

    inline bool msg_reply::derived_modify_message_body( scarab::param_node& a_node ) const
    {
        a_node.add( "retcode", f_return_code );
        a_node.add( "return_msg", f_return_msg );
        return true;
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

    inline bool msg_alert::derived_modify_amqp_message( amqp_message_ptr /*a_amqp_msg*/ ) const
    {
        return true;
    }

    inline bool msg_alert::derived_modify_message_body( scarab::param_node& /*a_node*/ ) const
    {
        return true;
    }

} /* namespace dripline */

#endif /* DRIPLINE_MESSAGE_HH_ */
