# Minimum cmake verison 3.1 required for the variable CMAKE_CXX_STANDARD
cmake_minimum_required (VERSION 3.1)

#########
# setup #
#########

cmake_policy( SET CMP0048 NEW ) # version in project()
project( Dripline VERSION 1.10.0 )

list( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/scarab/cmake )

include( PackageBuilder )

pbuilder_prepare_project()


####################
# dripline options #
####################

option( Dripline_BUILD_EXAMPLES "Flag to enable building the examples" FALSE )
option( Dripline_OFFLINE "Flag to enable offline use (primarily for testing)" FALSE )

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

################################
# Variables for parent project #
################################

pbuilder_variables_for_parent()

