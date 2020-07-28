# Minimum cmake verison 3.13 required for linking to libraries not created by the parent or sub directories
cmake_minimum_required (VERSION 3.1)

#########
# setup #
#########

cmake_policy( SET CMP0048 NEW ) # version in project()
project( Dripline VERSION 2.3.3 )

list( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/scarab/cmake )

include( PackageBuilder )

pbuilder_prepare_project()

# Get the dripline version
file( STRINGS "dripline_shield.json" JSON_STRINGS )
list( FILTER JSON_STRINGS INCLUDE REGEX "message" )
list( GET JSON_STRINGS 0 JSON_STRING )
string( REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" DRIPLINE_PROTOCOL_VERSION ${JSON_STRING})
message( STATUS "Dripline protocol version: ${DRIPLINE_PROTOCOL_VERSION}" )

pbuilder_expand_lib_name( Dripline )

####################
# dripline options #
####################

option( Dripline_BUILD_EXAMPLES "Flag to enable building the examples" FALSE )

option( Dripline_BUILD_PYTHON "Add the Python-dependent components" FALSE )

set ( Dripline_PYTHON_THROW_REPLY_KEYWORD "ThrowReply" CACHE STRING "Keyword used to indicate a throw_reply from Python (only used if building with dripline-python; should match the name of the class used in Python to throw replies)" )

set( Dripline_MAX_PAYLOAD_SIZE "10000" CACHE STRING "Maximum payload size (bytes)" )

# always use C++11
set_option( USE_CPP11 TRUE )


# Process options

if( Dripline_OFFLINE )
    add_definitions( -DDL_OFFLINE )
else()
    remove_definitions( -DDL_OFFLINE )
endif()

add_definitions( -DDL_MAX_PAYLOAD_SIZE=${Dripline_MAX_PAYLOAD_SIZE} )


#########################
# External dependencies #
#########################

set( PUBLIC_EXT_LIBS )
set( PRIVATE_EXT_LIBS )


####################
# SimpleAmqpClient #
####################

add_subdirectory( external/SimpleAmqpClient )

# Because the SimpleAmqpClient CMake build doesn't take care of this stuff, we have to do it ourselves

# Set interface include directories
set_property( TARGET SimpleAmqpClient 
    APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/SimpleAmqpClient/src>"
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

# Added to the public libraries so that downstream code can successfully get the necessary include directories
list( APPEND PUBLIC_EXT_LIBS SimpleAmqpClient )


##########
# Scarab #
##########

# set authentication build to TRUE
# this will require Boost 1.46 or better
set_option( Scarab_BUILD_AUTHENTICATION TRUE )
set_option( Scarab_BUILD_CODEC_JSON TRUE )

if( Dripline_BUILD_PYTHON )
    set_option( Scarab_BUILD_PYTHON TRUE )
else( Dripline_BUILD_PYTHON )
    set_option( Scarab_BUILD_PYTHON FALSE )
endif( Dripline_BUILD_PYTHON )

pbuilder_add_submodule( Scarab scarab )

if( Dripline_BUILD_PYTHON )
    message( STATUS "Dripline-cpp is being built with Python" )
    add_definitions( -DDL_PYTHON )
    add_definitions( -DPYTHON_THROW_REPLY_KEYWORD=${Dripline_PYTHON_THROW_REPLY_KEYWORD} )
    include_directories( ${PYTHON_INCLUDE_DIRS} )    
    #list( APPEND PRIVATE_EXT_LIBS Python::Python )
else( Dripline_BUILD_PYTHON )
    message( STATUS "Dripline-cpp is being built without Python" )
    remove_definitions( -DDL_PYTHON )
    remove_definitions( -DPYTHON_THROW_REPLY_KEYWORD=${Dripline_PYTHON_THROW_REPLY_KEYWORD} )
    #list( REMOVE_ITEM PRIVATE_EXT_LIBS Python::Python )
endif( Dripline_BUILD_PYTHON )

# all parts of dripline use Scarab, so include it here
pbuilder_use_sm_library( Scarab Scarab )


##################
# Build Dripline #
##################


#add_library( ${PROJECT_NAME}_void INTERFACE )
#export( TARGETS ${PROJECT_NAME}_void
#    FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake
#)
#install( TARGETS ${PROJECT_NAME}_void 
#    EXPORT ${PROJECT_NAME}Targets 
#    LIBRARY DESTINATION ${LIB_INSTALL_DIR} 
#)




add_subdirectory( library )

if( Dripline_ENABLE_EXECUTABLES )
    add_subdirectory( executables )
endif()

############
# Examples #
############

if( Dripline_BUILD_EXAMPLES )
    add_subdirectory( examples )
endif()

#########
# Tests #
#########

if( Dripline_ENABLE_TESTING )
    add_subdirectory( testing )
endif()

##################
# Package Config #
##################

configure_file( ${PROJECT_SOURCE_DIR}/DriplineConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/DriplineConfig.cmake @ONLY )

pbuilder_do_package_config()
