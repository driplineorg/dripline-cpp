# Minimum cmake verison 3.1 required for the variable CMAKE_CXX_STANDARD
cmake_minimum_required (VERSION 3.1)

#########
# setup #
#########

cmake_policy( SET CMP0048 NEW ) # version in project()
project( Dripline VERSION 2.3.2 )

list( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/scarab/cmake )

include( PackageBuilder )

pbuilder_prepare_project()

# Get the dripline version
file( STRINGS "dripline_shield.json" JSON_STRINGS )
list( FILTER JSON_STRINGS INCLUDE REGEX "message" )
list( GET JSON_STRINGS 0 JSON_STRING )
string( REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" DRIPLINE_PROTOCOL_VERSION ${JSON_STRING})
message( STATUS "Dripline protocol version: ${DRIPLINE_PROTOCOL_VERSION}" )


####################
# dripline options #
####################

option( Dripline_BUILD_EXAMPLES "Flag to enable building the examples" FALSE )

option( Dripline_BUILD_PYTHON "Add the Python-dependent components" FALSE )

set ( Dripline_PYTHON_THROW_REPLY_KEYWORD "ThrowReply" CACHE STRING "Keyword used to indicate a throw_reply from Python (only used if building with dripline-python; should match the name of the class used in Python to throw replies)" )

set( Dripline_MAX_PAYLOAD_SIZE "10000" CACHE STRING "Maximum payload size (bytes)" )

# always use C++11
set_option( USE_CPP11 TRUE )


#########################
# dripline dependencies #
#########################

########
# Scarab
########

# set authentication build to TRUE
# this will require Boost 1.46 or better
set_option( Scarab_BUILD_AUTHENTICATION TRUE )

pbuilder_add_submodule( Scarab scarab/library )

if( Dripline_BUILD_PYTHON )
    message( STATUS "Dripline-cpp is being built with Python" )
    set( Scarab_BUILD_PYTHON TRUE )
    add_definitions( -DDL_PYTHON )
    add_definitions( -DPYTHON_THROW_REPLY_KEYWORD=${Dripline_PYTHON_THROW_REPLY_KEYWORD} )
    include_directories( ${PYTHON_INCLUDE_DIRS} )    
else( Dripline_BUILD_PYTHON )
    message( STATUS "Dripline-cpp is being built without Python" )
    set( Scarab_BUILD_PYTHON FALSE )
    remove_definitions( -DDL_PYTHON )
    remove_definitions( -DPYTHON_THROW_REPLY_KEYWORD=${Dripline_PYTHON_THROW_REPLY_KEYWORD} )
endif( Dripline_BUILD_PYTHON )

##################
# SimpleAmqpClient
##################

pbuilder_add_submodule( SimpleAmqpClient SimpleAmqpClient )
set( Scarab_BUILD_CODEC_JSON TRUE CACHE BOOL "" FORCE )
set( Scarab_BUILD_CODEC_MSGPACK TRUE CACHE BOOL "" FORCE )


##################
# Build Dripline #
##################

if( Dripline_OFFLINE )
    add_definitions( -DDL_OFFLINE )
else()
    remove_definitions( -DDL_OFFLINE )
endif()

add_definitions( -DDL_MAX_PAYLOAD_SIZE=${Dripline_MAX_PAYLOAD_SIZE} )

include_directories( BEFORE
    ${PROJECT_SOURCE_DIR}/library
)

add_subdirectory( library )

if( Dripline_ENABLE_EXECUTABLES )
    add_subdirectory( executables )
endif()

############
# Examples #
############

if( Dripline_BUILD_EXAMPLES )
    include_directories( BEFORE
        ${PROJECT_SOURCE_DIR}/examples
    )

    add_subdirectory( examples )
endif()

#########
# Tests #
#########

if( Dripline_ENABLE_TESTING )
    add_subdirectory( testing )
endif()

#########################
# Package configuration #
#########################

install( EXPORT SimpleAmqpClientTargets
    DESTINATION ${LIB_INSTALL_DIR}/cmake/SimpleAmqpClientTargets
    NAMESPACE SimpleAmqpClient
    FILE SimpleAmqpClientConfig.cmake
)
install( EXPORT ScarabTargets
    DESTINATION ${LIB_INSTALL_DIR}/cmake/Scarab
    NAMESPACE Scarab
    FILE ScarabConfig.cmake
)
install( EXPORT DriplineTargets
    DESTINATION ${LIB_INSTALL_DIR}/cmake/DriplineCpp
    NAMESPACE DriplineCpp
    FILE DriplineCppConfig.cmake
)

include( CMakePackageConfigHelpers )

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/DriplineCppConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
)

install( FILES 
               ${CMAKE_CURRENT_BINARY_DIR}/DriplineCppConfigVersion.cmake
         DESTINATION ${LIB_INSTALL_DIR}/cmake/DriplineCpp
)


################################
# Variables for parent project #
################################

pbuilder_variables_for_parent()

