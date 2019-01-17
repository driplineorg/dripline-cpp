/*
 * dl_agent.cc
 *
 *      Author: Noah Oblath
 *
 *  Dripline agent
 *
 *  Usage:
 *  $> dl_agent [operation] [options]
 *
 */

#define DRIPLINE_API_EXPORTS
#define SCARAB_API_EXPORTS

#include "agent.hh"
#include "agent_config.hh"
#include "dripline_constants.hh"
#include "dripline_version.hh"

#include "application.hh"

using namespace dripline;

int main( int argc, char** argv )
{
    // Create the application and agent objects
    scarab::main_app the_main;
    agent the_agent;

    // Setup the application
    the_main.require_subcommand(1); // we use subcommands here
    the_main.fallthrough(); // pass any options not handled by the subcommands to the main app (this)

    // Routing key
    the_main.add_config_option< std::string >( "routing_key", "rk", "Set the routing key" )->required();

    // Specifier
    the_main.add_config_option< std::string >( "specifier", "specifier", "Set the specifier" );

    // Application subcommands
    scarab::app* t_agent_run = the_main.add_subcommand( "run", "Send an OP_RUN request" );
    t_agent_run->callback(
            [&]() { the_agent.execute< agent::sub_agent_run >( the_main.master_config(), the_main.nonoption_ord_args() ); }
    );

    scarab::app* t_agent_get = the_main.add_subcommand( "get", "Send an OP_GET request" );
    t_agent_get->callback(
            [&]() { the_agent.execute< agent::sub_agent_get >( the_main.master_config(), the_main.nonoption_ord_args() ); }
    );

    scarab::app* t_agent_set = the_main.add_subcommand( "set", "Send an OP_SET request" );
    t_agent_set->callback(
            [&]() { the_agent.execute< agent::sub_agent_set >( the_main.master_config(), the_main.nonoption_ord_args() ); }
    );

    scarab::app* t_agent_cmd = the_main.add_subcommand( "cmd", "Send an OP_CMD request" );
    t_agent_cmd->callback(
            [&]() { the_agent.execute< agent::sub_agent_cmd>( the_main.master_config(), the_main.nonoption_ord_args() ); }
    );

    // Default configuration
    the_main.default_config() = agent_config();

    // Command line options
    the_main.add_config_option< std::string >( "-b,--broker", "amqp.broker", "Set the dripline broker address" );
    the_main.add_config_option< unsigned >( "-p,--port", "amqp.broker-port", "Set the port for communication with the dripline broker" );
    the_main.add_config_option< std::string >( "-e,--exchange", "amqp.exchange", "Set the exchange to send message on" );
    the_main.add_config_option< std::string >( "-a,--auth-file", "amqp.auth-file" "Set the authentication file path" );
    the_main.add_config_option< unsigned >( "-t,--timeout", "amqp.timeout", "Set the timeout for waiting for a reply (seconds)" );
    the_main.add_config_option< std::string >( "-k,--lockout-key", "lockout-key", "Set the lockout key to send with the message" );
    // TODO: --payload [vec string, key=value]
    // TODO: -v,--values [vec string, values]

    // Package version
    the_main.set_version( new dripline::version() );

    // Parse CL options and run the application
    CLI11_PARSE( the_main, argc, argv );

    return RETURN_SUCCESS;
}

