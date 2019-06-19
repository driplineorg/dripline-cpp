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

    the_main.footer( std::string() + "\nValues and Keyword Arguments:\n\n" +
            "  Further positional arguments (other than the routing key and specifier)\n" +
            "  can be used to add to the values array in the payload and to other parts of the payload.\n\n" +
            "  Arguments in the form [key]=[value] will be assumed to be keyword arguments.\n" +
            "  Other arguments will be assumed to be entries in the values array.\n\n" +
            "  The key portion of a keyword argument is an address that can specify\n" +
            "  both node and array locations (e.g. my.value.0)." );

    // Routing key
    the_main.add_config_option< std::string >( "routing_key", "rk", "Set the routing key" )->required();

    // Specifier
    the_main.add_config_option< std::string >( "specifier", "specifier", "Set the specifier" );

    // Application subcommands
    scarab::config_decorator* t_agent_run = the_main.add_config_subcommand( "run", "Send an OP_RUN request" );
    t_agent_run->this_app()->callback(
            [&]() { the_agent.execute< agent::sub_agent_run >( the_main.master_config(), the_main.nonoption_ord_args() ); }
    );

    scarab::config_decorator* t_agent_get = the_main.add_config_subcommand( "get", "Send an OP_GET request" );
    t_agent_get->this_app()->callback(
            [&]() { the_agent.execute< agent::sub_agent_get >( the_main.master_config(), the_main.nonoption_ord_args() ); }
    );

    scarab::config_decorator* t_agent_set = the_main.add_config_subcommand( "set", "Send an OP_SET request" );
    t_agent_set->this_app()->callback(
            [&]() { the_agent.execute< agent::sub_agent_set >( the_main.master_config(), the_main.nonoption_ord_args() ); }
    );

    scarab::config_decorator* t_agent_cmd = the_main.add_config_subcommand( "cmd", "Send an OP_CMD request" );
    t_agent_cmd->this_app()->callback(
            [&]() { the_agent.execute< agent::sub_agent_cmd>( the_main.master_config(), the_main.nonoption_ord_args() ); }
    );

    // Default configuration
    the_main.default_config() = agent_config();

    // Command line options
    the_main.add_config_option< std::string >( "-b,--broker", "amqp.broker", "Set the dripline broker address" );
    the_main.add_config_option< unsigned >( "-p,--port", "amqp.broker-port", "Set the port for communication with the dripline broker" );
    the_main.add_config_option< std::string >( "-e,--exchange", "amqp.exchange", "Set the exchange to send message on" );
    the_main.add_config_option< std::string >( "-a,--auth-file", "amqp.auth-file", "Set the authentication file path" );
    the_main.add_config_option< unsigned >( "-t,--timeout", "amqp.timeout", "Set the timeout for waiting for a reply (seconds)" );
    the_main.add_config_option< std::string >( "-k,--lockout-key", "lockout-key", "Set the lockout key to send with the message" );
    the_main.add_config_flag< bool >( "--suppress-output", "agent.suppress-output", "Suppress the output of the returned reply" );
    the_main.add_config_flag< bool >( "--pretty-print", "agent.pretty-print", "Output the returned reply in nicely formatted JSON" );
    the_main.add_config_multi_option< std::string >( "-P,--payload", "payload", "Add values to the payload" );
    the_main.add_config_multi_option< std::string >( "-v,--values", "option-values", "Add ordered values" ); // stored in the config as "option-values" so they can be merged in later in the proper order
    the_main.add_config_flag< bool >( "--dry-run-msg", "dry-run-msg", "Print the message contents and exit" );

    // Package version
    the_main.set_version( new dripline::version() );

    // Parse CL options and run the application
    CLI11_PARSE( the_main, argc, argv );

    return the_agent.get_return() / 100; // this exit code is the class of the dripline return code
}

