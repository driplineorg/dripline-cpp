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

                    void execute( const scarab::param_node& a_config );

                    virtual request_ptr_t create_request( scarab::param_node& a_config ) = 0;
                    virtual request_ptr_t create_request();

                protected:
                    agent* f_agent;
            };

            class sub_agent_run : public sub_agent
            {
                public:
                    sub_agent_run( agent* an_agent ) : sub_agent( an_agent ) {}
                    virtual ~sub_agent_run() {}

                    virtual request_ptr_t create_request( scarab::param_node& a_config );
            };

            class sub_agent_get : public sub_agent
            {
                public:
                    sub_agent_get( agent* an_agent ) : sub_agent( an_agent ) {}
                    virtual ~sub_agent_get() {}

                    virtual request_ptr_t create_request( scarab::param_node& a_config );
            };

            class sub_agent_set : public sub_agent
            {
                public:
                    sub_agent_set( agent* an_agent ) : sub_agent( an_agent ) {}
                    virtual ~sub_agent_set() {}

                    virtual request_ptr_t create_request( scarab::param_node& a_config );
            };

            class sub_agent_cmd : public sub_agent
            {
                public:
                    sub_agent_cmd( agent* an_agent ) : sub_agent( an_agent ) {}
                    virtual ~sub_agent_cmd() {}

                    virtual request_ptr_t create_request( scarab::param_node& a_config );
            };

        public:
            agent();
            virtual ~agent();

            template< typename sub_agent_type >
            void execute( const scarab::param_node& a_config );

            //mv_referrable( scarab::param_node, config );

            mv_referrable( std::string, routing_key );
            mv_referrable( std::string, specifier );
            mv_referrable( uuid_t, lockout_key );

            mv_accessible( reply_ptr_t, reply );

            mv_accessible( int, return );

    };

    template< typename sub_agent_type >
    void agent::execute( const scarab::param_node& a_config )
    {
        sub_agent_type t_sub_agent( this );
        t_sub_agent.execute( a_config );
        return;
    }

    inline request_ptr_t agent::sub_agent::create_request()
    {
        scarab::param_node t_config;
        return create_request( t_config );
    }

    inline void agent::set_opt_broker( const std::string& a_broker )
    {
        f_app_options["amqp"]["broker"]() = a_broker;
        return;
    }

} /* namespace dripline */

#endif /* DRIPLINE_AGENT_HH_ */
