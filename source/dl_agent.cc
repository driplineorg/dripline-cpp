/*
Copyright 2016 Noah S. Oblath

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

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

#include "configurator.hh"
#include "logger.hh"

using namespace dripline;


LOGGER( dlog, "mantis_client" );

set_version( dripline, version );

void print_usage();
void print_version();

int main( int argc, char** argv )
{
    dripline::version_setter s_vsetter_mantis_version( new dripline::version() );
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
}


void print_usage()
{
    std::stringstream t_use_stream;
    t_use_stream << "dl_agent: utility for sending dripline requests\n"
                 << "===============================================\n\n";

    t_use_stream << "Use\n"
                 << "---\n\n"
                 << "  > dl_agent [command] [options]\n\n";

    t_use_stream << "Commands\n"
                 << "--------\n\n"
            << "  The four dripline commands are available as:\n"
            << "   - run\n"
            << "   - get\n"
            << "   - set\n"
            << "   - cmd\n\n";

    t_use_stream << "Options\n"
                 << "-------\n\n"
            << " - Routing key: rk=[routing key] (required)\n"
            << " - Value for set: value=[a_value] (required for ``set``)\n"
            << " - Broker address: amqp.broker=[address] (default is ``localhost``)\n"
            << " - Broker port: amqp.broker-port=[port] (default is 5672)\n"
            << " - Exchange: amqp.exchange=[exchange] (default is ``requests``)\n"
            << " - Reply timeout: amqp.reply-timeout-ms=[ms] (default is ``10000``)\n"
            << " - Authentications file: amqp.auth-file=[filename] (default is none)\n"
            << " - Lockout key: lockout-key=[uuid]\n"
            << " - Filename to save reply: save=[filename] (optional)\n"
            << " - Filename for payload: load=[filename] (optional)\n";

    LINFO( dlog, "\n\n"
           << t_use_stream.str() );

    return;
}

void print_version()
{
    dripline::version t_dripver;
    scarab::version t_scarabver;
    LINFO( dlog, "\n\n"
           << "You're using: " << t_dripver.exe_name() << '\n'
           << t_dripver.version_info_string() << '\n'
           << t_scarabver.version_info_string() );
    return;
}
