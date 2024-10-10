# CMakeLists.txt for dripline-cpp/testing
# Author: N. Oblath
# Created: Aug 14, 2018

##########

# Install Catch2

include( FetchContent )

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)

FetchContent_MakeAvailable( Catch2 )
list( APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras )

##########

#set( testing_HEADERS
#)

set( testing_SOURCES
    run_dl_tests.cc
    test_agent.cc
    test_amqp.cc
    test_core.cc
    test_dripline_error.cc
    test_endpoint.cc
    test_lockout.cc
    test_messages.cc
    test_return_codes.cc
    test_scheduler.cc
    test_service.cc
    test_specifier.cc
    test_throw_reply.cc
    test_uuid.cc
    test_version_store.cc
)

set( testing_LIBS 
    Dripline
)

if( Dripline_BUILD_EXAMPLES )
    set( testing_SOURCES
        ${testing_SOURCES}
        test_examples_oscillator.cc
    )

    set( testing_LIBS
        ${testing_LIBS}
        DriplineExamples
    )
endif()

pbuilder_executable( 
    EXECUTABLE run_dl_tests
    SOURCES ${testing_SOURCES}
    PROJECT_LIBRARIES ${testing_LIBS}
    PRIVATE_EXTERNAL_LIBRARIES Catch2::Catch2
)

list( APPEND programs "run_dl_tests" )

pbuilder_component_install_and_export( 
    COMPONENT Catch2
    LIBTARGETS Catch2
    NAMESPACE Catch2::
)

pbuilder_component_install_and_export( 
    COMPONENT Executables
    EXETARGETS ${programs}
)


# So far it seems sufficient to use Catch2's ability to run tests, and not CTest.
# Here are two ways it could potentially be integrated with CTest, if we change our minds.

# Normal CMake Integration
#enable_testing()
#add_test(NAME test_test COMMAND run_tests )

# Automated CMake Integration
#include(CTest)
#include(Catch)
#catch_discover_tests(foo)
