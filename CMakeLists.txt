# Minimum cmake verison 3.1 required for the variable CMAKE_CXX_STANDARD
cmake_minimum_required (VERSION 3.1)

#########
# setup #
#########

cmake_policy( SET CMP0048 NEW ) # version in project()
project( Dripline VERSION 1.3.0 )

list( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/scarab/cmake )

include( PackageBuilder )

pbuilder_prepare_project()


####################
# dripline options #
####################

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

include_directories( BEFORE
    ${PROJECT_SOURCE_DIR}/source
)

add_subdirectory( source )

################################
# Variables for parent project #
################################

pbuilder_variables_for_parent()

