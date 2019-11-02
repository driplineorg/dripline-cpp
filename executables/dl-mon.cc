/*!
   @file dl-mon.cc
   @author N.S. Oblath
  
   @brief Dripline agent: send messages and receive replies from the command line
  
   @details
   Usage:
   ~~~~
   $> dl-mon [options] [-r [request keys]] [-a [alert keys]]
   ~~~~
  
   Use `dl-mon -h` to get the full help information.
*/

#include "dripline_constants.hh"
#include "monitor.hh"
#include "monitor_config.hh"
#include "version_store.hh"

#include "application.hh"
#include "logger.hh"

using namespace dripline;

LOGGER( dlog, "dl-mon" );

int main( int argc, char** argv )
{
    // Create the application
    scarab::main_app the_main;

    // Setup the application

    // Default configuration
    the_main.default_config() = monitor_config();

    try
    {
        // Command line options
        add_dripline_options( the_main );
        the_main.add_config_multi_option< std::string >( "-r,--requests", "request-keys", "Assign keys for binding to the requests exchange" );
        the_main.add_config_multi_option< std::string >( "-a,--alerts", "alert-keys", "Assign keys for binding to the alerts exchange" );
        the_main.add_config_flag< bool >( "--json-print", "json-print", "Output the returned reply in JSON; default is white-space suppressed (see --pretty-print)" );
        the_main.add_config_flag< bool >( "--pretty-print", "pretty-print", "Output the returned reply in nicely formatted JSON" );
    }
    catch( std::exception& e )
    {
        LERROR( dlog, "Error setting up the command-line options: " << e.what() );
        return dl_client_error().rc_value();
    }

    // Package version
    the_main.set_version( version_store::get_instance()->versions().at("dripline-cpp") );

    // main callback function and return code extraction
    int the_return = -1;
    auto t_callback = [&](){
        the_return = dl_client_error().rc_value() / 100;

        auto the_monitor = std::make_shared< monitor >( the_main.master_config() );

        if( ! the_monitor->start() ) return;

        if( ! the_monitor->listen() ) return;

        if( ! the_monitor->stop() ) return;

        the_return = dl_success().rc_value() / 100;
    };

    the_main.callback( t_callback );

    // Parse CL options and run the application
    CLI11_PARSE( the_main, argc, argv );

    return the_return;
}



