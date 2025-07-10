# Minimum cmake verison 3.12 required by Scarab
cmake_minimum_required (VERSION 3.12)

#########
# setup #
#########

cmake_policy( SET CMP0048 NEW ) # version in project()
project( Dripline VERSION 2.10.6 )

list( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/scarab/cmake )

include( PackageBuilder )

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

# always use C++17 or higher
set_to_max( CMAKE_CXX_STANDARD 17 )

# This can be used to debug memory issues
#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

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

# Pybind11
if( Dripline_BUILD_PYTHON )
    find_package( Python3 REQUIRED COMPONENTS Interpreter Development )
    set(PYBIND11_PYTHON_VERSION ${Python3_VERSION} CACHE STRING "")
    find_package( pybind11 REQUIRED )
    include_directories( ${Python3_INCLUDE_DIRS} )
    list( APPEND PUBLIC_EXT_LIBS pybind11::module pybind11::embed )
endif( Dripline_BUILD_PYTHON )

# Threads are required by: test_scheduler
if( Dripline_ENABLE_TESTING )
    find_package( Threads REQUIRED )
    list( APPEND PUBLIC_EXT_LIBS Threads::Threads )
    set( THREADS_PREFER_PTHREAD_FLAG TRUE )
endif()


####################
# SimpleAmqpClient #
####################

# SimpleAmqpClient needs to be built independently from, but as a submodule and dependency of, dripline-cpp.  
# In other words, the configuration and build environments need to be kept separate.

# First we run the configure stage of SimpleAmqpClient during the configure (cmake) process of dripline-cpp.
# This build takes place in its own build directory, but it installs in the same place as dripline-cpp.
# We do this here instead of below in the ExternalProject so that dripline-cpp can learn about the SimpleAmqpClient targets at the configure stage.

if( NOT EXISTS ${PROJECT_BINARY_DIR}/external )
    execute_process( 
        COMMAND mkdir external
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
endif()

execute_process( 
    COMMAND cmake -D CMAKE_INSTALL_PREFIX:STRING=${CMAKE_INSTALL_PREFIX} 
                    -D PACKAGE_CONFIG_PREFIX:STRING=${PACKAGE_CONFIG_PREFIX}
                    -D TOP_PROJECT_CMAKE_CONFIG_DIR:STRING=${TOP_PROJECT_CMAKE_CONFIG_DIR}
                    -D LIB_INSTALL_DIR:STRING=${LIB_INSTALL_DIR}
                    -D BIN_INSTALL_DIR:STRING=${BIN_INSTALL_DIR}
                    -D CMAKE_BUILD_TYPE=RELEASE
                    -D CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                    ${PROJECT_SOURCE_DIR}/external
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/external
)

# Second we use the ExternalProject module to build and install SimpleAmqpClient.
# This takes place during the build phase of dripline-cpp.
# Note that we disable the configure stage of the ExternalProject because we already took care of the configure stage above.
include(ExternalProject)

ExternalProject_Add( DriplineExternal
    PREFIX            ${CMAKE_INSTALL_PREFIX}
    SOURCE_DIR        ${PROJECT_SOURCE_DIR}/external
    BINARY_DIR        ${PROJECT_BINARY_DIR}/external
    CONFIGURE_COMMAND ""
    BUILD_ALWAYS      1
    BUILD_COMMAND     $(MAKE)
    INSTALL_DIR       ${CMAKE_INSTALL_PREFIX}
    INSTALL_COMMAND   $(MAKE) install
)

# Finally we use find_package() to learn about the Kassiopeia build targets.
find_package( SimpleAmqpClient REQUIRED CONFIG HINTS ${PROJECT_BINARY_DIR}/external )

# Added to the public libraries so that downstream code can successfully get the necessary include directories
list( APPEND PUBLIC_EXT_LIBS SimpleAmqpClient::SimpleAmqpClient )


##################
# Scarab Options #
##################

# set authentication build to TRUE
# this will require Boost 1.46 or better
set_option( Scarab_BUILD_AUTHENTICATION TRUE )
set_option( Scarab_BUILD_CODEC_JSON TRUE )

if( Dripline_BUILD_PYTHON )
    set_option( Scarab_BUILD_PYTHON TRUE )
else( Dripline_BUILD_PYTHON )
    set_option( Scarab_BUILD_PYTHON FALSE )
endif( Dripline_BUILD_PYTHON )

# SimpleAmqpClient needs system and chrono
list( APPEND Scarab_BOOST_COMPONENTS system chrono )
 

#####################
# Prepare for Build #
#####################

pbuilder_prepare_project()

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

pbuilder_do_package_config(
    INPUT_FILE ${PROJECT_SOURCE_DIR}/DriplineConfig.cmake.in
    OUTPUT_FILE DriplineConfig.cmake
)
