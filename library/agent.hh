/*
 * agent.hh
 *
 *  Created on: Jun 2, 2016
 *      Author: nsoblath
 *
 */

#ifndef DRIPLINE_AGENT_HH_
#define DRIPLINE_AGENT_HH_

#include "message.hh"

#include "param.hh"


namespace scarab
{
    class authentication;
}
namespace dripline
{
    class core;

    /*!
     @class agent
     @author N.S. Oblath

     @brief Takes command-line arguments and sends messages

     @details
     The primary purpose of this class is to send messages.  It can send requests, alerts, or replies.
     It is optimized for taking command-line arguments and translating them into a message to send.

     It uses the subcommand approach to the command-line interface.  Each type of message to send is 
     encapsulated in a particular subclass of the internal class `sub_agent`.  Each type of `sub_agent` 
     has an `execute()` function that performs the subcommand.

     CL arguments are translated to messages to send via the configuration param_node that is passed to 
     the subcommand.  The expected configuration is:

     ~~~
     {
         "[operation: run, get, set, cmd, alert, reply] : "",
         "rk" : "[routing key]",
         "dripline_mesh" : {
             "broker" : "[address]",
             "broker_port" : [port],
             "exchange" : "[exchange]",
             "reply_timeout_ms": [ms] // optional; default is 10000
         },
         "auth-file" : "[filename]" // optional, if using an auth file
         "auth-groups" : {
            "dripline": {
                "username": {[username specification]}
                "password": {[password specification]}
            }
         }
         "lockout_key" : "[uuid]",  // optional
         "save" : "[filename]"  // optional
         "load" : "[filename]"  // optional; only used for cmd
         "return" : { // used only for replies
             "code" : [return code],
             "message" : "[return message]"
         }
         "pretty_print" : null // optional; if present sets output to nicely formatted JSON
         "suppress_output" : null // optional; if present suppresses the normal agent output
         "dry_run_agent" : null // optional; if present prints the message to be sent and exits
     }
     ~~~

    */
    class DRIPLINE_API agent
    {
        public:
            class DRIPLINE_API sub_agent
            {
                public:
                    sub_agent( agent* an_agent ) : f_agent( an_agent ) {};
                    sub_agent( const sub_agent& ) = default;
                    sub_agent( sub_agent&& ) = default;
                    virtual ~sub_agent() = default;

                    sub_agent& operator=( const sub_agent& ) = default;
                    sub_agent& operator=( sub_agent&& ) = default;

                    void execute( const scarab::param_node& a_config, const scarab::authentication& a_auth );
                    void execute( const scarab::param_node& a_config, const scarab::param_array& a_ord_args, const scarab::authentication& a_auth );

                    virtual void create_and_send_message( scarab::param_node& a_config, const core& a_core ) = 0;
                    virtual void create_and_send_message( const core& a_core );

                protected:
                    agent* f_agent;
            };

            class DRIPLINE_API sub_agent_request : public sub_agent
            {
                public:
                    sub_agent_request( agent* an_agent ) : sub_agent( an_agent ) {}
                    sub_agent_request( const sub_agent_request& ) = default;
                    sub_agent_request( sub_agent_request&& ) = default;
                    virtual ~sub_agent_request() = default;

                    sub_agent_request& operator=( const sub_agent_request& ) = default;
                    sub_agent_request& operator=( sub_agent_request&& ) = default;

                    virtual void create_and_send_message( scarab::param_node& a_config, const core& a_core );

                    virtual request_ptr_t create_request( scarab::param_node& a_config ) = 0;
            };

            class DRIPLINE_API sub_agent_reply : public sub_agent
            {
                public:
                    sub_agent_reply( agent* an_agent ) : sub_agent( an_agent ) {}
                    sub_agent_reply( const sub_agent_reply& ) = default;
                    sub_agent_reply( sub_agent_reply&& ) = default;
                    virtual ~sub_agent_reply() = default;

                    sub_agent_reply& operator=( const sub_agent_reply& ) = default;
                    sub_agent_reply& operator=( sub_agent_reply&& ) = default;

                    virtual void create_and_send_message( scarab::param_node& a_config, const core& a_core );
            };

            class DRIPLINE_API sub_agent_alert : public sub_agent
            {
                public:
                    sub_agent_alert( agent* an_agent ) : sub_agent( an_agent ) {}
                    sub_agent_alert( const sub_agent_alert& ) = default;
                    sub_agent_alert( sub_agent_alert&& ) = default;
                    virtual ~sub_agent_alert() = default;

                    sub_agent_alert& operator=( const sub_agent_alert& ) = default;
                    sub_agent_alert& operator=( sub_agent_alert&& ) = default;

                    virtual void create_and_send_message( scarab::param_node& a_config, const core& a_core );
            };

            class DRIPLINE_API sub_agent_run : public sub_agent_request
            {
                public:
                    sub_agent_run( agent* an_agent ) : sub_agent_request( an_agent ) {}
                    sub_agent_run( const sub_agent_run& ) = default;
                    sub_agent_run( sub_agent_run&& ) = default;
                    virtual ~sub_agent_run() = default;

                    sub_agent_run& operator=( const sub_agent_run& ) = default;
                    sub_agent_run& operator=( sub_agent_run&& ) = default;

                    virtual request_ptr_t create_request( scarab::param_node& a_config );
            };

            class DRIPLINE_API sub_agent_get : public sub_agent_request
            {
                public:
                    sub_agent_get( agent* an_agent ) : sub_agent_request( an_agent ) {}
                    sub_agent_get( const sub_agent_get& ) = default;
                    sub_agent_get( sub_agent_get&& ) = default;
                    virtual ~sub_agent_get() = default;

                    sub_agent_get& operator=( const sub_agent_get& ) = default;
                    sub_agent_get& operator=( sub_agent_get&& ) = default;

                    virtual request_ptr_t create_request( scarab::param_node& a_config );
            };

            class DRIPLINE_API sub_agent_set : public sub_agent_request
            {
                public:
                    sub_agent_set( agent* an_agent ) : sub_agent_request( an_agent ) {}
                    sub_agent_set( const sub_agent_set& ) = default;
                    sub_agent_set( sub_agent_set&& ) = default;
                    virtual ~sub_agent_set() = default;

                    sub_agent_set& operator=( const sub_agent_set& ) = default;
                    sub_agent_set& operator=( sub_agent_set&& ) = default;

                    virtual request_ptr_t create_request( scarab::param_node& a_config );
            };

            class DRIPLINE_API sub_agent_cmd : public sub_agent_request
            {
                public:
                    sub_agent_cmd( agent* an_agent ) : sub_agent_request( an_agent ) {}
                    sub_agent_cmd( const sub_agent_cmd& ) = default;
                    sub_agent_cmd( sub_agent_cmd&& ) = default;
                    virtual ~sub_agent_cmd() = default;

                    sub_agent_cmd& operator=( const sub_agent_cmd& ) = default;
                    sub_agent_cmd& operator=( sub_agent_cmd&& ) = default;

                    virtual request_ptr_t create_request( scarab::param_node& a_config );
            };

        public:
            agent();
            agent( const agent& ) = default;
            agent( agent&& ) = default;
            virtual ~agent() = default;

            agent& operator=( const agent& ) = default;
            agent& operator=( agent&& ) = default;

            template< typename sub_agent_type >
            void execute( const scarab::param_node& a_config, const scarab::authentication& a_auth );
            template< typename sub_agent_type >
            void execute( const scarab::param_node& a_config, const scarab::param_array& a_ord_args, const scarab::authentication& a_auth );

            mv_accessible( bool, is_dry_run );

            // all message types
            mv_referrable( std::string, routing_key );
            mv_referrable( std::string, specifier );

            // requests only
            mv_referrable( uuid_t, lockout_key );

            // alerts only
            mv_accessible( unsigned, return_code );
            mv_referrable( std::string, return_message );

            // use only for requests
            mv_accessible( unsigned, timeout );
            mv_accessible( bool, suppress_output );
            mv_accessible( bool, json_print );
            mv_accessible( bool, pretty_print );
            mv_referrable( std::string, save_filename );

            mv_accessible( reply_ptr_t, reply );

            mv_accessible( int, return );

    };

    template< typename sub_agent_type >
    void agent::execute( const scarab::param_node& a_config, const scarab::authentication& a_auth )
    {
        sub_agent_type t_sub_agent( this );
        scarab::param_array t_ord_args;
        t_sub_agent.execute( a_config, t_ord_args, a_auth );
        return;
    }

    template< typename sub_agent_type >
    void agent::execute( const scarab::param_node& a_config, const scarab::param_array& a_ord_args, const scarab::authentication& a_auth )
    {
        sub_agent_type t_sub_agent( this );
        t_sub_agent.execute( a_config, a_ord_args, a_auth );
        return;
    }

    inline void agent::sub_agent::create_and_send_message( const core& a_core )
    {
        scarab::param_node t_config;
        create_and_send_message( t_config, a_core );
        return;
    }

} /* namespace dripline */

#endif /* DRIPLINE_AGENT_HH_ */
