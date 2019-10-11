/*
 * hub.hh
 *
 *  Created on: Jan 7, 2016
 *      Author: nsoblath
 */

#ifndef DRIPLINE_HUB_HH_
#define DRIPLINE_HUB_HH_

#include "service.hh"

#include "dripline_exceptions.hh"

#include <unordered_map>

namespace dripline
{
    class DRIPLINE_API hub : public service
    {
        private:
            typedef std::function< reply_ptr_t( const dripline::request_ptr_t ) > handler_func_t;

        public:
            hub( const scarab::param_node& a_config = scarab::param_node(), const std::string& a_queue_name = "",  const std::string& a_broker_address = "", unsigned a_port = 0, const std::string& a_auth_file = "" , const bool a_make_connection = true );
            virtual ~hub();

            void set_run_handler( const handler_func_t& a_func );
            void register_get_handler( const std::string& a_key, const handler_func_t& a_func );
            void register_set_handler( const std::string& a_key, const handler_func_t& a_func );
            void register_cmd_handler( const std::string& a_key, const handler_func_t& a_func );

            void remove_get_handler( const std::string& a_key );
            void remove_set_handler( const std::string& a_key );
            void remove_cmd_handler( const std::string& a_key );

        private:
            //*************************
            // Hub request distributors
            //*************************

            virtual reply_ptr_t do_run_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_get_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_set_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_cmd_request( const request_ptr_t a_request );

            handler_func_t f_run_handler;

            typedef std::unordered_map< std::string, handler_func_t > handler_funcs_t;
            handler_funcs_t f_get_handlers;
            handler_funcs_t f_set_handlers;
            handler_funcs_t f_cmd_handlers;

    };

} /* namespace dripline */

#endif /* DRIPLINE_HUB_HH_ */
