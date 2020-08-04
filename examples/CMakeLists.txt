# CMakeLists.txt for dripline-cpp/examples
# Author: N. Oblath
# Created: Aug 24, 2018

##########

include_directories( BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set( dripline_examples_HEADERS
    oscillator.hh
    oscillator_service_endpoints.hh
    oscillator_service_hub.hh
    simple_service.hh
)

set( dripline_examples_SOURCES
    oscillator.cc
    oscillator_service_endpoints.cc
    oscillator_service_hub.cc
    simple_service.cc
)

set ( dripline_examples_LIBS
    Dripline
)

##########

pbuilder_library( 
    TARGET DriplineExamples 
    SOURCES ${dripline_examples_SOURCES}
    PROJECT_LIBRARIES ${dripline_examples_LIBS}
    #PUBLIC_EXTERNAL_LIBRARIES ${PUBLIC_EXT_LIBS}
    #PRIVATE_EXTERNAL_LIBRARIES ${PRIVATE_EXT_LIBS}
)

pbuilder_install_headers( ${dripline_examples_HEADERS} )

if( Dripline_ENABLE_EXECUTABLES )
    set( dripline_examples_exe_LIBS 
        Dripline
        DriplineExamples
    )
    set( dripline_examples_exe_SOURCES
        run_oscillator_service_endpoints.cc
        run_oscillator_service_hub.cc
        run_simple_service.cc
    )

    set( dripline_examples_exe_PROGRAMS )
    pbuilder_executables( 
        SOURCES ${dripline_examples_exe_SOURCES}
        TARGETS_VAR dripline_examples_exe_PROGRAMS
        PROJECT_LIBRARIES ${dripline_examples_exe_LIBS}
        #PUBLIC_EXTERNAL_LIBRARIES ${PUBLIC_EXT_LIBS}
        #PRIVATE_EXTERNAL_LIBRARIES ${PRIVATE_EXT_LIBS}
    )
endif()

pbuilder_component_install_and_export( 
    COMPONENT Examples
    LIBTARGETS DriplineExamples
    EXETARGETS ${dripline_examples_exe_PROGRAMS}
)
