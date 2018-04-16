/*
 * hub.hh
 *
 *  Created on: Jan 7, 2016
 *      Author: nsoblath
 */

#ifndef DRIPLINE_HUB_HH_
#define DRIPLINE_HUB_HH_

#include "service.hh"

#include "dripline_error.hh"

#include <unordered_map>

namespace dripline
{
    struct DRIPLINE_API reply_package
    {
        const service* f_service_ptr;
        std::string f_reply_to;
        std::string f_correlation_id;
        scarab::param_node f_payload;
        reply_package( const service* a_service, request_ptr_t a_request );
        bool send_reply( retcode_t a_return_code, const std::string& a_return_msg ) const;
        bool send_reply( const dripline_error& an_error ) const;
    };

    class DRIPLINE_API hub : public service
    {
        private:
            typedef std::function< bool( const dripline::request_ptr_t, dripline::reply_package& ) > handler_func_t;

        public:
            hub( const scarab::param_node* a_config = nullptr, const std::string& a_queue_name = "",  const std::string& a_broker_address = "", unsigned a_port = 0, const std::string& a_auth_file = "" , const bool a_make_connection = true );
            virtual ~hub();

            void set_run_handler( const handler_func_t& a_func );
            void register_get_handler( const std::string& a_key, const handler_func_t& a_func );
            void register_set_handler( const std::string& a_key, const handler_func_t& a_func );
            void register_cmd_handler( const std::string& a_key, const handler_func_t& a_func );

            void remove_get_handler( const std::string& a_key );
            void remove_set_handler( const std::string& a_key );
            void remove_cmd_handler( const std::string& a_key );

        private:
            /// Handle request messages
            virtual bool on_request_message( const request_ptr_t a_request );

            //*****************************
            // Default request distributors
            //*****************************

            // Override the relevant function to implement use of that type of message

            virtual bool do_run_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            virtual bool do_get_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            virtual bool do_set_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            virtual bool do_cmd_request( const request_ptr_t a_request, reply_package& a_reply_pkg );

            handler_func_t f_run_handler;

            typedef std::unordered_map< std::string, handler_func_t > handler_funcs_t;
            handler_funcs_t f_get_handlers;
            handler_funcs_t f_set_handlers;
            handler_funcs_t f_cmd_handlers;

        private:
            bool __do_run_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            bool __do_get_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            bool __do_set_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            bool __do_cmd_request( const request_ptr_t a_request, reply_package& a_reply_pkg );

        private:
            //*****************
            // Request handlers
            //*****************

            bool handle_lock_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            bool handle_unlock_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            bool handle_is_locked_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            bool handle_ping_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            bool handle_set_condition_request( const request_ptr_t a_request, reply_package& a_reply_pkg );

        public:
            //******************
            // Lockout functions
            //******************

            /// enable lockout with randomly-generated key
            uuid_t enable_lockout( const scarab::param_node& a_tag );
            /// enable lockout with user-supplied key
            uuid_t enable_lockout( const scarab::param_node& a_tag, uuid_t a_key );
            bool disable_lockout( const uuid_t& a_key, bool a_force = false );

            bool is_locked() const;
            const scarab::param_node& get_lockout_tag() const;
            bool check_key( const uuid_t& a_key ) const;

        private:
            // Returns true if the server is unlocked or if it's locked and the key matches the lockout key; returns false otherwise.
            bool authenticate( const uuid_t& a_key ) const;

            virtual bool __do_handle_set_condition_request( const request_ptr_t a_request, reply_package& a_reply_pkg );

            scarab::param_node f_lockout_tag;
            uuid_t f_lockout_key;

    };

    inline bool reply_package::send_reply( const dripline_error& an_error ) const
    {
        return send_reply( an_error.retcode(), an_error.what() );
    }

    inline uuid_t hub::enable_lockout( const scarab::param_node& a_tag )
    {
        return enable_lockout( a_tag, generate_random_uuid() );
    }

    inline bool hub::is_locked() const
    {
        return ! f_lockout_key.is_nil();
    }

    inline const scarab::param_node& hub::get_lockout_tag() const
    {
        return f_lockout_tag;
    }

    inline bool hub::check_key( const uuid_t& a_key ) const
    {
        return f_lockout_key == a_key;
    }

    inline bool hub::__do_handle_set_condition_request( const request_ptr_t, reply_package& a_reply_pkg )
    {
        return a_reply_pkg.send_reply( retcode_t::success, "No action taken (default method)" );
    }

} /* namespace dripline */

#endif /* DRIPLINE_HUB_HH_ */
