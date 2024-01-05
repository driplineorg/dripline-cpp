# This is a wrapper project used to build any external dependencies that require wrapping.
#
# SimpleAmqpClient requires wrapping so that modern CMake practices (use of targets and interfaces) can be applied.

# Minimum cmake verison 3.1 required by dripline-cpp
cmake_minimum_required (VERSION 3.1)

cmake_policy( SET CMP0048 NEW ) # version in project()
project( DriplineExternal VERSION 0.0.0 )

##################
# SimpleAmqpClient
##################

option( BUILD_API_DOCS "" OFF )
add_subdirectory( SimpleAmqpClient )

# Because the SimpleAmqpClient CMake build doesn't take care of this stuff, we have to do it ourselves

# Set interface include directories
set_property( TARGET SimpleAmqpClient 
    APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/SimpleAmqpClient/src>"
            "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>"
)

# Build-interface targets
export( TARGETS SimpleAmqpClient
    NAMESPACE SimpleAmqpClient::
    FILE ${CMAKE_CURRENT_BINARY_DIR}/SimpleAmqpClientTargets.cmake
)

# Install-interface targets
# The SimpleAmqpClientTargets export was added to SimpleAmqpClient in the project8 fork
# Install SimpleAmqpClientTargets.cmake in the Dripline package-config install directory
install( EXPORT SimpleAmqpClientTargets
    FILE
        SimpleAmqpClientTargets.cmake
    NAMESPACE
        SimpleAmqpClient::
    DESTINATION
        ${PACKAGE_CONFIG_PREFIX}/SimpleAmqpClient
)

# Process SimpleAmqpClient.cmake.in
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/SimpleAmqpClientConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/SimpleAmqpClientConfig.cmake @ONLY )

install( 
    FILES 
        ${CMAKE_CURRENT_BINARY_DIR}/SimpleAmqpClientConfig.cmake
    DESTINATION 
        ${TOP_PROJECT_CMAKE_CONFIG_DIR}/SimpleAmqpClient
)

