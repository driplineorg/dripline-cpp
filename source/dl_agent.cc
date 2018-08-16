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

#include "application.hh"


#include "agent.hh"
#include "agent_config.hh"
#include "dripline_constants.hh"
#include "dripline_version.hh"

#include "logger.hh"

using namespace dripline;


LOGGER( dlog, "mantis_client" );

int main( int argc, char** argv )
{
    scarab::main_app the_main;
    the_main.require_subcommand();
    the_main.fallthrough();

    the_main.set_version( new dripline::version() );

    // options
    //std::string t_broker;
    //the_main.add_option( "-b,--broker", , "RabbitMQ broker" );



    agent the_agent;

    // subcommands
    scarab::app* t_agent_run = the_main.add_subcommand( "run", "Send an OP_RUN request" );
    t_agent_run->callback(
            [&the_agent]() { the_agent.do_run(); }
    );

    scarab::app* t_agent_get = the_main.add_subcommand( "get", "Send an OP_GET request" );
    t_agent_run->callback(
            [&the_agent]() { the_agent.do_get(); }
    );

    scarab::app* t_agent_set = the_main.add_subcommand( "set", "Send an OP_SET request" );
    t_agent_run->callback(
            [&the_agent]() { the_agent.do_set(); }
    );

    scarab::app* t_agent_cmd = the_main.add_subcommand( "cmd", "Send an OP_CMD request" );
    t_agent_run->callback(
            [&the_agent]() { the_agent.do_cmd(); }
    );

    CLI11_PARSE( the_main, argc, argv );

    return RETURN_SUCCESS;





/*

    try
    {
        agent_config t_cc;
        scarab::configurator t_configurator( argc, argv, t_cc );

        if( t_configurator.help_flag() )
        {
            print_usage();
            return RETURN_SUCCESS;
        }

        if( t_configurator.version_flag() )
        {
            print_version();
            return RETURN_SUCCESS;
        }

        // Run the client

        agent the_agent( t_configurator.config() );

        the_agent.execute();

        return the_agent.get_return();
    }
    catch( scarab::error& e )
    {
        LERROR( dlog, "configuration error: " << e.what() );
        return RETURN_ERROR;
    }
    catch( std::exception& e )
    {
        LERROR( dlog, "std::exception caught: " << e.what() );
        return RETURN_ERROR;
    }

    return RETURN_ERROR;
    */
}

