/*
 * run_dl_tests.cc
 *
 *  Created on: Oct 10, 2024
 *      Author: N.S. Oblath
 */


#include "catch2/catch_session.hpp"

int main( int argc, char* argv[] ) {

  int result = Catch::Session().run( argc, argv );

  return result;
}
