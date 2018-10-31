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
    the_main.require_subcommand(); // we use subcommands here
    the_main.fallthrough(); // pass any options not handled by the subcommands to the main app (this)

    // Applicaton subcommands
    scarab::app* t_agent_run = the_main.add_subcommand( "run", "Send an OP_RUN request" );
    t_agent_run->callback(
            [this]() { the_agent.execute< agent::sub_agent_run >( the_main.master_config() ); }
    );

    scarab::app* t_agent_get = the_main.add_subcommand( "get", "Send an OP_GET request" );
    t_agent_get->callback(
            [this]() { the_agent.execute< agent::sub_agent_get >( the_main.master_config() ); }
    );

    scarab::app* t_agent_set = the_main.add_subcommand( "set", "Send an OP_SET request" );
    t_agent_set->callback(
            [this]() { the_agent.execute< agent::sub_agent_set >( the_main.master_config() ); }
    );

    scarab::app* t_agent_cmd = the_main.add_subcommand( "cmd", "Send an OP_CMD request" );
    t_agent_cmd->callback(
            [this]() { the_agent.execute< agent::sub_agent_cmd>( the_main.master_config() ); }
    );

    // Default configuration
    the_main.default_config() = agent_config();

    // Command line options
    // TODO: agent needs to know about the different specific CL options, but main_app is where the app_options param_node is
    the_main.add_option( "-b,--broker", [&](std::vector< std::string > args){ set_opt_broker( args[0] ); return true; }, "Set the dripline broker" );

    // Package version
    the_main.set_version( new dripline::version() );

    // Parse CL options and run the application
    CLI11_PARSE( the_main, argc, argv );

    return RETURN_SUCCESS;
}

