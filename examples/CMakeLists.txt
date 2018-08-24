# CMakeLists.txt for dripline-cpp/examples
# Author: N. Oblath
# Created: Aug 24, 2018

##########

set( dripline_examples_HEADERS
    simple_service.hh
)

set( dripline_examples_SOURCES
    simple_service.cc
)

set ( dripline_examples_LIBS
    Dripline
)

##########

pbuilder_library( DriplineExamples dripline_examples_SOURCES dripline_examples_LIBS )

pbuilder_install_headers( ${dripline_examples_HEADERS} )

if( Dripline_ENABLE_EXECUTABLES )
    set( lib_dependencies 
        Dripline
        DriplineExamples
    )
    set( programs
        run_simple_service
    )
    pbuilder_executables( programs lib_dependencies )
endif( Dripline_ENABLE_EXECUTABLES )
