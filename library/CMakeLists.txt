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
    return_codes.hh
    service.hh
    specifier.hh
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
    return_codes.cc
    service.cc
    specifier.cc
    uuid.cc
)

if( Dripline_BUILD_FOR_LABVIEW )
    set( dripline_HEADERS
        ${dripline_HEADERS}
        labview_service.hh
    )

    set( dripline_SOURCES
        ${dripline_SOURCES}
        labview_service.cc
    )
endif( Dripline_BUILD_FOR_LABVIEW )

configure_file( dripline_version.cc.in ${CMAKE_CURRENT_BINARY_DIR}/dripline_version.cc )
set( dripline_SOURCES
    ${dripline_SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/dripline_version.cc
)


##########

pbuilder_library( Dripline dripline_SOURCES "" )

pbuilder_install_headers( ${dripline_HEADERS} )
