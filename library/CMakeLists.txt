# CMakeLists.txt for dripline-cpp/library
# Author: N. Oblath
# Created: Jan 5, 2016

##########

include_directories( BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set( dripline_HEADERS
    agent.hh
    agent_config.hh
    amqp.hh
    core.hh
    dripline_api.hh
    dripline_config.hh
    dripline_constants.hh
    dripline_exceptions.hh
    dripline_fwd.hh
    dripline_version.hh
    endpoint.hh
    heartbeater.hh
    hub.hh
    listener.hh
    message.hh
    monitor.hh
    monitor_config.hh
    receiver.hh
    relayer.hh
    return_codes.hh
    scheduler.hh
    service.hh
    service_config.hh
    specifier.hh
    throw_reply.hh
    uuid.hh
    version_store.hh
)

set( dripline_SOURCES
    agent.cc
    agent_config.cc
    amqp.cc
    core.cc
    dripline_config.cc
    dripline_constants.cc
    dripline_version.cc
    endpoint.cc
    heartbeater.cc
    hub.cc
    listener.cc
    message.cc
    monitor.cc
    monitor_config.cc
    receiver.cc
    relayer.cc
    return_codes.cc
    service.cc
    service_config.cc
    specifier.cc
    throw_reply.cc
    uuid.cc
    version_store.cc
)

if( Scarab_BUILD_PYTHON )
    set( dripline_HEADERS 
        ${dripline_HEADERS}
        reply_cache.hh
    )

    set( dripline_SOURCES
        ${dripline_SOURCES}
        reply_cache.cc
    )
endif( Scarab_BUILD_PYTHON )

configure_file( dripline_version.cc.in ${CMAKE_CURRENT_BINARY_DIR}/dripline_version.cc )
set( dripline_SOURCES
    ${dripline_SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/dripline_version.cc
)

set( PACKAGE_LIBS )


##########

pbuilder_library( 
    TARGET Dripline 
    SOURCES ${dripline_SOURCES}
    PUBLIC_EXTERNAL_LIBRARIES ${PUBLIC_EXT_LIBS}
    PRIVATE_EXTERNAL_LIBRARIES ${PRIVATE_EXT_LIBS}
)

pbuilder_component_install_and_export( 
    COMPONENT Library
    LIBTARGETS Dripline
    NAMESPACE Dripline::
)

pbuilder_install_headers( ${dripline_HEADERS} )
