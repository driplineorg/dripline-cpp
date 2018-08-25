# CMakeLists.txt for dripline-cpp/library
# Author: N. Oblath
# Created: Jan 5, 2016

##########

set( dripline_HEADERS
    agent.hh
    agent_config.hh
    amqp.hh
    core.hh
    dripline_api.hh
    dripline_constants.hh
    dripline_error.hh
    dripline_version.hh
    endpoint.hh
    hub.hh
    message.hh
    relayer.hh
    reply_package.hh
    return_codes.hh
    routing_key_specifier.hh
    service.hh
    uuid.hh
)

set( dripline_SOURCES
    agent.cc
    agent_config.cc
    core.cc
    dripline_constants.cc
    dripline_error.cc
    dripline_version.cc
    endpoint.cc
    hub.cc
    message.cc
    relayer.cc
    reply_package.cc
    return_codes.cc
    routing_key_specifier.cc
    service.cc
    uuid.cc
)

configure_file( dripline_version.cc.in ${CMAKE_CURRENT_BINARY_DIR}/dripline_version.cc )
set( dripline_SOURCES
    ${dripline_SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/dripline_version.cc
)


##########

pbuilder_library( Dripline dripline_SOURCES "" )

pbuilder_install_headers( ${dripline_HEADERS} )
