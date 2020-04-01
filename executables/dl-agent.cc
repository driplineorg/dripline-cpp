/*!
   @file dl-agent.cc
   @author N.S. Oblath
  
   @brief Dripline agent: send messages and receive replies from the command line
  
   @details
   Usage:
   ~~~~
   $> dl-agent [options] [operation] [routing key] [values]
   ~~~~
  
   Use `dl-agent -h` to get the full help information.
*/

#include "agent.hh"
#include "agent_config.hh"
#include "dripline_constants.hh"
#include "version_store.hh"

#include "application.hh"
#include "logger.hh"

using namespace dripline;

int main( int argc, char** argv )
{
    // Switch the logger out stream to std::cerr
    scarab::logger::SetOutStream( &std::cerr );

    // Create the application and agent objects
    scarab::main_app the_main;
    agent the_agent;

    // Setup the application
    the_main.require_subcommand(1); // we use subcommands here
    the_main.fallthrough(); // pass any options not handled by the subcommands to the main app (this)

    the_main.footer( std::string() + "\nValues and Keyword Arguments:\n\n" +
            "  Positional arguments can be used to add to the values array in the payload\n" +
            "  and to other parts of the payload.\n\n" +
            "  Arguments in the form [key]=[value] will be assumed to be keyword arguments.\n" +
            "  Other arguments will be assumed to be entries in the values array.\n\n" +
            "  The key portion of a keyword argument is an address that can specify\n" +
            "  both node and array locations (e.g. my.value.0)." );

    // Application subcommands
    scarab::config_decorator* t_agent_get = the_main.add_config_subcommand( "get", "Send an OP_GET request" );
    t_agent_get->add_config_option< std::string >( "routing_key", "rk", "Set the routing key" )->required();
    t_agent_get->this_app()->callback(
            [&]() { the_agent.execute< agent::sub_agent_get >( the_main.master_config(), the_main.nonoption_ord_args() ); }
    );

    scarab::config_decorator* t_agent_set = the_main.add_config_subcommand( "set", "Send an OP_SET request" );
    t_agent_set->add_config_option< std::string >( "routing_key", "rk", "Set the routing key" )->required();
    t_agent_set->this_app()->callback(
            [&]() { the_agent.execute< agent::sub_agent_set >( the_main.master_config(), the_main.nonoption_ord_args() ); }
    );

    scarab::config_decorator* t_agent_cmd = the_main.add_config_subcommand( "cmd", "Send an OP_CMD request" );
    t_agent_cmd->add_config_option< std::string >( "routing_key", "rk", "Set the routing key" )->required();
    t_agent_cmd->this_app()->callback(
            [&]() { the_agent.execute< agent::sub_agent_cmd>( the_main.master_config(), the_main.nonoption_ord_args() ); }
    );

    scarab::config_decorator* t_agent_reply = the_main.add_config_subcommand( "reply", "Send a reply message" );
    t_agent_reply->add_config_option< std::string >( "routing_key", "rk", "Set the routing key" )->required();
    t_agent_reply->this_app()->callback(
            [&]() { the_agent.execute< agent::sub_agent_reply>( the_main.master_config(), the_main.nonoption_ord_args() ); }
    );

    scarab::config_decorator* t_agent_alert = the_main.add_config_subcommand( "alert", "Send an alert message" );
    t_agent_alert->add_config_option< std::string >( "routing_key", "rk", "Set the routing key" )->required();
    t_agent_alert->this_app()->callback(
            [&]() { the_agent.execute< agent::sub_agent_alert>( the_main.master_config(), the_main.nonoption_ord_args() ); }
    );

    // Default configuration
    the_main.default_config() = agent_config();

    // Command line options
    add_dripline_options( the_main );
    the_main.add_config_option< std::string >( "-s,--specifier", "specifier", "Set the specifier" );
    the_main.add_config_multi_option< std::string >( "-P,--payload", "payload", "Add values to the payload" );
    the_main.add_config_multi_option< std::string >( "--values", "option-values", "Add ordered values" ); // stored in the config as "option-values" so they can be merged in later in the proper order
    the_main.add_config_option< unsigned >( "-t,--timeout", "timeout", "Set the timeout for waiting for a reply (seconds)" );
    the_main.add_config_option< std::string >( "-k,--lockout-key", "lockout-key", "Set the lockout key to send with the message (for sending requests only)" );
    the_main.add_config_flag< bool >( "--suppress-output", "suppress-output", "Suppress the output of the returned reply" );
    the_main.add_config_flag< bool >( "--json-print", "json-print", "Output the returned reply in JSON; default is white-space suppressed (see --pretty-print)" );
    the_main.add_config_flag< bool >( "--pretty-print", "pretty-print", "Output the returned reply in nicely formatted JSON" );
    the_main.add_config_option< unsigned >( "--return-code", "return.code", "Set the return code sent (for sending replies only)" );
    the_main.add_config_option< std::string >( "--return-msg", "return.message", "Set the return message sent (for sending replies only)" );
    the_main.add_config_flag< bool >( "--dry-run-msg", "dry-run-msg", "Print the message contents and exit" );

    // Package version
    the_main.set_version( version_store::get_instance()->versions().at("dripline-cpp") );

    // Parse CL options and run the application
    CLI11_PARSE( the_main, argc, argv );

    return the_agent.get_return() / 100; // this exit code is the class of the dripline return code
}

