# CMakeLists.txt for dripline-cpp/testing
# Author: N. Oblath
# Created: Aug 14, 2018

##########

#set( testing_HEADERS
#)

set( testing_SOURCES
    use_catch.cc
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
    test_version_store.cc
)

set( testing_LIBS 
    Dripline
    Catch
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

# Interface library was recommended for Catch:
# https://stackoverflow.com/questions/34896891/catch-lib-unit-testing-and-ctest-cmake-integration/34900012#34900012
add_library( Catch INTERFACE )
target_include_directories( Catch INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/catch )

add_executable( run_tests ${testing_SOURCES} )
target_link_libraries( run_tests ${testing_LIBS} )

# So far it seems sufficient to use Catch2's ability to run tests, and not CTest.
# Here are two ways it could potentially be integrated with CTest, if we change our minds.

# Normal CMake Integration
#enable_testing()
#add_test(NAME test_test COMMAND run_tests )

# Automated CMake Integration
#include(CTest)
#include(Catch)
#catch_discover_tests(foo)
