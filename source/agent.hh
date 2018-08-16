/*
 * agent.hh
 *
 *  Created on: Jun 2, 2016
 *      Author: nsoblath
 *
 *  Expected configuration supplied to the constructor:
 *
 *  {
 *      "[operation: run, get, set, or cmd] : "",
 *      "amqp" : {
 *          "broker" : "[address]",
 *          "broker-port" : [port],
 *          "exchange" : "[exchange]",
 *          "auth-file" : "[authentication file]",  // optional; must live in the user's home directory
 *          "reply-timeout-ms": [ms] // optional; default is 10000
 *      },
 *      "rk" : "[routing key]",
 *      "lockout-key" : "[uuid]",  // optional
 *      "save" : "[filename]"  // optional
 *      "load" : "[filename]"  // optional; only used for cmd
 *  }
 */

#ifndef DRIPLINE_AGENT_HH_
#define DRIPLINE_AGENT_HH_

#include "message.hh"

#include "param.hh"


namespace dripline
{
    class DRIPLINE_API agent
    {
        public:
            agent();
            virtual ~agent();

            void do_run( const scarab::param_node& a_node );
            void do_get( const scarab::param_node& a_node );
            void do_set( const scarab::param_node& a_node );
            void do_cmd( const scarab::param_node& a_node );

            void execute( const scarab::param_node& a_node );
            //void cancel();

            mv_referrable( scarab::param_node, config );

            mv_referrable( std::string, routing_key );
            mv_accessible( uuid_t, lockout_key );

            mv_accessible_noset( reply_ptr_t, reply );

            mv_accessible_noset( int, return );

        private:
            typedef std::function< request_ptr_t() > create_request_t;
            create_request_t f_create_request_ptr;

            request_ptr_t create_run_request();
            request_ptr_t create_get_request();
            request_ptr_t create_set_request();
            request_ptr_t create_cmd_request();
    };

} /* namespace dripline */

#endif /* DRIPLINE_AGENT_HH_ */
