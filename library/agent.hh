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
         "amqp" : {
             "broker" : "[address]",
             "broker-port" : [port],
             "exchange" : "[exchange]",
             "auth-file" : "[authentication file]",  // optional; must live in the user's home directory
             "reply-timeout-ms": [ms] // optional; default is 10000
         },
         "lockout-key" : "[uuid]",  // optional
         "save" : "[filename]"  // optional
         "load" : "[filename]"  // optional; only used for cmd
         "return" : { // used only for replies
             "code" : [return code],
             "message" : "[return message]"
         }
         "pretty-print" : null // optional; if present sets output to nicely formatted JSON
         "suppress-output" : null // optional; if present suppresses the normal agent output
         "dry-run-agent" : null // optional; if present prints the message to be sent and exits
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
                    virtual ~sub_agent() {};

                    void execute( const scarab::param_node& a_config );
                    void execute( const scarab::param_node& a_config, const scarab::param_array& a_ord_args );

                    virtual void create_and_send_message( scarab::param_node& a_config, const core& a_core ) = 0;
                    virtual void create_and_send_message( const core& a_core );

                protected:
                    agent* f_agent;
            };

            class DRIPLINE_API sub_agent_request : public sub_agent
            {
                public:
                    sub_agent_request( agent* an_agent ) : sub_agent( an_agent ) {}
                    virtual ~sub_agent_request() {}

                    virtual void create_and_send_message( scarab::param_node& a_config, const core& a_core );

                    virtual request_ptr_t create_request( scarab::param_node& a_config ) = 0;
            };

            class DRIPLINE_API sub_agent_reply : public sub_agent
            {
                public:
                    sub_agent_reply( agent* an_agent ) : sub_agent( an_agent ) {}
                    virtual ~sub_agent_reply() {}

                    virtual void create_and_send_message( scarab::param_node& a_config, const core& a_core );
            };

            class DRIPLINE_API sub_agent_alert : public sub_agent
            {
                public:
                    sub_agent_alert( agent* an_agent ) : sub_agent( an_agent ) {}
                    virtual ~sub_agent_alert() {}

                    virtual void create_and_send_message( scarab::param_node& a_config, const core& a_core );
            };

            class DRIPLINE_API sub_agent_run : public sub_agent_request
            {
                public:
                    sub_agent_run( agent* an_agent ) : sub_agent_request( an_agent ) {}
                    virtual ~sub_agent_run() {}

                    virtual request_ptr_t create_request( scarab::param_node& a_config );
            };

            class DRIPLINE_API sub_agent_get : public sub_agent_request
            {
                public:
                    sub_agent_get( agent* an_agent ) : sub_agent_request( an_agent ) {}
                    virtual ~sub_agent_get() {}

                    virtual request_ptr_t create_request( scarab::param_node& a_config );
            };

            class DRIPLINE_API sub_agent_set : public sub_agent_request
            {
                public:
                    sub_agent_set( agent* an_agent ) : sub_agent_request( an_agent ) {}
                    virtual ~sub_agent_set() {}

                    virtual request_ptr_t create_request( scarab::param_node& a_config );
            };

            class DRIPLINE_API sub_agent_cmd : public sub_agent_request
            {
                public:
                    sub_agent_cmd( agent* an_agent ) : sub_agent_request( an_agent ) {}
                    virtual ~sub_agent_cmd() {}

                    virtual request_ptr_t create_request( scarab::param_node& a_config );
            };

        public:
            agent();
            virtual ~agent();

            template< typename sub_agent_type >
            void execute( const scarab::param_node& a_config );
            template< typename sub_agent_type >
            void execute( const scarab::param_node& a_config, const scarab::param_array& a_ord_args );

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
    void agent::execute( const scarab::param_node& a_config )
    {
        sub_agent_type t_sub_agent( this );
        scarab::param_array t_ord_args;
        t_sub_agent.execute( a_config, t_ord_args );
        return;
    }

    template< typename sub_agent_type >
    void agent::execute( const scarab::param_node& a_config, const scarab::param_array& a_ord_args )
    {
        sub_agent_type t_sub_agent( this );
        t_sub_agent.execute( a_config, a_ord_args );
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
