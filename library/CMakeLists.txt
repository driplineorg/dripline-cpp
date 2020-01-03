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
    dripline_config.hh
    dripline_constants.hh
    dripline_exceptions.hh
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
    dripline_exceptions.cc
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


##########

pbuilder_library( Dripline dripline_SOURCES "" )

pbuilder_install_headers( ${dripline_HEADERS} )
