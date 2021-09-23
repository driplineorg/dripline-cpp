# CMakeLists.txt for dripline-cpp/testing/integration
# Author: N. Oblath
# Created: Aug 31, 2021

##########

#set( testing_HEADERS
#)

set( testing_SOURCES
    use_catch.cc
    test_int_channel.cc
    test_int_service.cc
)

set( testing_LIBS 
    Dripline
    Catch
)

add_executable( run_int_tests ${testing_SOURCES} )
target_link_libraries( run_int_tests ${testing_LIBS} )
 
# So far it seems sufficient to use Catch2's ability to run tests, and not CTest.
# Here are two ways it could potentially be integrated with CTest, if we change our minds.

# Normal CMake Integration
#enable_testing()
#add_test(NAME test_test COMMAND run_tests )

# Automated CMake Integration
#include(CTest)
#include(Catch)
#catch_discover_tests(foo)
