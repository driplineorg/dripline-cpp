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
            class sub_agent
            {
                public:
                    sub_agent( agent* an_agent ) : f_agent( an_agent ) {};
                    virtual ~sub_agent() {};

                    void execute( const scarab::param_node& a_node );
                    virtual request_ptr_t create_request() = 0;

                protected:
                    agent* f_agent;
            };

            class sub_agent_run : public sub_agent
            {
                public:
                    sub_agent_run( agent* an_agent ) : sub_agent( an_agent ) {}
                    virtual ~sub_agent_run() {}

                    virtual request_ptr_t create_request();
            };

            class sub_agent_get : public sub_agent
            {
                public:
                    sub_agent_get( agent* an_agent ) : sub_agent( an_agent ) {}
                    virtual ~sub_agent_get() {}

                    virtual request_ptr_t create_request();
            };

            class sub_agent_set : public sub_agent
            {
                public:
                    sub_agent_set( agent* an_agent ) : sub_agent( an_agent ) {}
                    virtual ~sub_agent_set() {}

                    virtual request_ptr_t create_request();
            };

            class sub_agent_cmd : public sub_agent
            {
                public:
                    sub_agent_cmd( agent* an_agent ) : sub_agent( an_agent ) {}
                    virtual ~sub_agent_cmd() {}

                    virtual request_ptr_t create_request();
            };

        public:
            agent();
            virtual ~agent();

            template< typename sub_agent_type >
            void execute( const scarab::param_node& a_node );

            mv_referrable( scarab::param_node, config );

            mv_referrable( std::string, routing_key );
            mv_referrable( std::string, specifier );
            mv_referrable( uuid_t, lockout_key );

            mv_accessible( reply_ptr_t, reply );

            mv_accessible( int, return );

    };

    template< typename sub_agent_type >
    void agent::execute( const scarab::param_node& a_node )
    {
        sub_agent_type t_sub_agent( this );
        t_sub_agent.execute( a_node );
        return;
    }

} /* namespace dripline */

#endif /* DRIPLINE_AGENT_HH_ */
