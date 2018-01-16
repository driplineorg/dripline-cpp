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
#include "parsable.hh"

#include "amqp.hh"
#include "dripline_api.hh"
#include "dripline_constants.hh"
#include "routing_key_specifier.hh"
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

        protected:
            virtual bool derived_modify_amqp_message( amqp_message_ptr t_amqp_msg ) const = 0;
            virtual bool derived_modify_message_body( scarab::param_node& a_node ) const = 0;

            bool encode_message_body( std::string& a_body ) const;
            std::string interpret_encoding() const;

        public:
            mv_referrable( std::string, routing_key );
            mv_referrable( std::string, rks );
            //mv_referrable( routing_key_specifier, parsed_rks );
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

        private:
            mutable routing_key_specifier f_parsed_rks;

        public:
            routing_key_specifier& parsed_rks();
            const routing_key_specifier& parsed_rks() const;

            bool set_routing_key_specifier( const std::string& a_rks, const routing_key_specifier& a_parsed_rks );

            virtual msg_t message_type() const = 0;

            void set_sender_info( scarab::param_node* a_payload );
            const scarab::param_node& get_sender_info() const;
            scarab::param_node& get_sender_info();

            void set_sender_package( const std::string& a_pkg );

            void set_sender_exe( const std::string& a_exe );

            void set_sender_version( const std::string& a_vsn );

            void set_sender_commit( const std::string& a_cmt );

            void set_sender_hostname( const std::string& a_host );

            void set_sender_username( const std::string& a_user );

            void set_sender_service_name( const std::string& a_service );

            void set_payload( scarab::param_node* a_payload );
            const scarab::param_node& get_payload() const;
            scarab::param_node& get_payload();

        protected:
            scarab::param_node* f_sender_info;
            scarab::param_node* f_payload;
    };

    DRIPLINE_API std::ostream& operator<<( std::ostream& a_os, message::encoding a_enc );


    //***********
    // Request
    //***********

    class DRIPLINE_API msg_request : public message
    {
        public:
            msg_request();
            virtual ~msg_request();

            static request_ptr_t create( scarab::param_node* a_payload, op_t a_msg_op, const std::string& a_routing_key, const std::string& a_reply_to, message::encoding a_encoding = encoding::json );

            bool is_request() const;
            bool is_reply() const;
            bool is_alert() const;

            reply_ptr_t reply( retcode_t a_ret_code, const std::string& a_ret_msg ) const;

        private:
            bool derived_modify_amqp_message( amqp_message_ptr t_amqp_msg ) const;
            bool derived_modify_message_body( scarab::param_node& a_node ) const;

        public:
            virtual msg_t message_type() const;
            static msg_t get_message_type();

            mv_referrable( uuid_t, lockout_key );
            mv_accessible( bool, lockout_key_valid );
            mv_accessible( op_t, message_op );

        private:
            static msg_t s_message_type;

    };


    //*********
    // Reply
    //*********

    class DRIPLINE_API msg_reply : public message
    {
        public:
            msg_reply();
            virtual ~msg_reply();

            static reply_ptr_t create( retcode_t a_retcode, const std::string& a_ret_msg, scarab::param_node* a_payload, const std::string& a_routing_key, message::encoding a_encoding = encoding::json );
            static reply_ptr_t create( const dripline_error& a_error, const std::string& a_routing_key, message::encoding a_encoding = encoding::json );

            bool is_request() const;
            bool is_reply() const;
            bool is_alert() const;

        private:
            bool derived_modify_amqp_message( amqp_message_ptr t_amqp_msg ) const;
            bool derived_modify_message_body( scarab::param_node& a_node ) const;

        public:
            virtual msg_t message_type() const;
            static msg_t get_message_type();

            mv_accessible( retcode_t, return_code );
            mv_referrable( std::string, return_msg );

        private:
            mutable std::string f_return_buffer;

            static msg_t s_message_type;

    };

    //*********
    // Alert
    //*********

    class DRIPLINE_API msg_alert : public message
    {
        public:
            msg_alert();
            virtual ~msg_alert();

            static alert_ptr_t create( scarab::param_node* a_payload, const std::string& a_routing_key, message::encoding a_encoding = encoding::json );

            bool is_request() const;
            bool is_reply() const;
            bool is_alert() const;

        private:
            bool derived_modify_amqp_message( amqp_message_ptr t_amqp_msg ) const;
            bool derived_modify_message_body( scarab::param_node& a_node ) const;

        public:
            virtual msg_t message_type() const;
            static msg_t get_message_type();

        private:
            static msg_t s_message_type;

    };


    //***********
    // Message
    //***********

    inline void message::set_sender_info( scarab::param_node* a_sender_info )
    {
        delete f_sender_info;
        f_sender_info = a_sender_info;
        if( ! f_sender_info->has( "package" ) ) f_sender_info->add( "package", new scarab::param_value( "N/A" ) );
        f_sender_package = f_sender_info->get_value( "package" );
        if( ! f_sender_info->has( "exe" ) ) f_sender_info->add( "exe", new scarab::param_value( "N/A" ) );
        f_sender_exe = f_sender_info->get_value( "exe" );
        if( ! f_sender_info->has( "version" ) ) f_sender_info->add( "version", new scarab::param_value( "N/A" ) );
        f_sender_version = f_sender_info->get_value( "version" );
        if( ! f_sender_info->has( "commit" ) ) f_sender_info->add( "commit", new scarab::param_value( "N/A" ) );
        f_sender_commit = f_sender_info->get_value( "commit" );
        if( ! f_sender_info->has( "hostname" ) ) f_sender_info->add( "hostname", new scarab::param_value( "N/A" ) );
        f_sender_hostname = f_sender_info->get_value( "hostname" );
        if( ! f_sender_info->has( "username" ) ) f_sender_info->add( "username", new scarab::param_value( "N/A" ) );
        f_sender_username = f_sender_info->get_value( "username" );
        if( ! f_sender_info->has( "service_name" ) ) f_sender_info->add( "service_name", new scarab::param_value( "N/A" ) );
        f_sender_service_name = f_sender_info->get_value( "service_name" );
    }

    inline const scarab::param_node& message::get_sender_info() const
    {
        return *f_sender_info;
    }

    inline scarab::param_node& message::get_sender_info()
    {
        return *f_sender_info;
    }

    inline void message::set_sender_package( const std::string& a_pkg )
    {
        f_sender_info->value_at( "package" ).set( a_pkg );
        f_sender_package = a_pkg;
        return;
    }

    inline void message::set_sender_exe( const std::string& a_exe )
    {
        f_sender_info->value_at( "exe" ).set( a_exe );
        f_sender_exe = a_exe;
        return;
    }

    inline void message::set_sender_version( const std::string& a_vsn )
    {
        f_sender_info->value_at( "version" ).set( a_vsn );
        f_sender_version = a_vsn;
        return;
    }

    inline void message::set_sender_commit( const std::string& a_cmt )
    {
        f_sender_info->value_at( "commit" ).set( a_cmt );
        f_sender_commit = a_cmt;
        return;
    }

    inline void message::set_sender_hostname( const std::string& a_host )
    {
        f_sender_info->value_at( "hostname" ).set( a_host );
        f_sender_hostname = a_host;
        return;
    }

    inline void message::set_sender_username( const std::string& a_user )
    {
        f_sender_info->value_at( "username" ).set( a_user );
        f_sender_username = a_user;
        return;
    }

    inline void message::set_sender_service_name( const std::string& a_service )
    {
        f_sender_info->value_at( "service_name" ).set( a_service );
        f_sender_service_name = a_service;
        return;
    }

    inline void message::set_payload( scarab::param_node* a_payload )
    {
        delete f_payload;
        f_payload = a_payload;
        return;
    }

    inline const scarab::param_node& message::get_payload() const
    {
        return *f_payload;
    }

    inline scarab::param_node& message::get_payload()
    {
        return *f_payload;
    }

    inline routing_key_specifier& message::parsed_rks()
    {
        return f_parsed_rks;
    }

    const inline routing_key_specifier& message::parsed_rks() const
    {
        return f_parsed_rks;
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
        a_node.add( "msgop", new scarab::param_value( to_uint(f_message_op) ) );
        a_node.add( "lockout_key", new scarab::param_value( string_from_uuid( lockout_key() ) ) );
        return true;
    }

    inline reply_ptr_t msg_request::reply( retcode_t a_ret_code, const std::string& a_ret_msg ) const
    {
        reply_ptr_t t_reply = std::make_shared< msg_reply >();
        t_reply->set_return_code( a_ret_code );
        t_reply->return_msg() = a_ret_msg;
        t_reply->correlation_id() = f_correlation_id;
        t_reply->routing_key() = f_reply_to;
        return t_reply;
    }


    //*********
    // Reply
    //*********

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
        a_node.add( "retcode", new scarab::param_value( to_uint(f_return_code) ) );
        a_node.add( "return_msg", new scarab::param_value( f_return_msg ) );
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
