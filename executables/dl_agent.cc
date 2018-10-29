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
#include "dripline_constants.hh"
#include "dripline_version.hh"

using namespace dripline;

int main( int argc, char** argv )
{
    agent the_main;

    the_main.set_version( new dripline::version() );

    CLI11_PARSE( the_main, argc, argv );

    return RETURN_SUCCESS;
}

