# CMakeLists.txt for dripline-cpp/executables
# Author: N. Oblath
# Created: Jan 5, 2016

##########

set( dripline_exe_SOURCES
    dl-agent.cc
    dl-mon.cc
)

set( dripline_exe_LIBS 
    Dripline
)


set( dripline_exe_PROGRAMS )
pbuilder_executables( 
    SOURCES ${dripline_exe_SOURCES}
    TARGETS_VAR dripline_exe_PROGRAMS
    PROJECT_LIBRARIES ${dripline_exe_LIBS}
    #PUBLIC_EXTERNAL_LIBRARIES ${PUBLIC_EXT_LIBS}
    #PRIVATE_EXTERNAL_LIBRARIES ${PRIVATE_EXT_LIBS}
)

pbuilder_component_install_and_export( 
    COMPONENT Executables
    EXETARGETS ${dripline_exe_PROGRAMS}
)
