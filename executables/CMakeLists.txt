# CMakeLists.txt for dripline-cpp/executables
# Author: N. Oblath
# Created: Jan 5, 2016

##########

set( sources
    dl-agent.cc
    dl-mon.cc
)

set( PACKAGE_LIBS 
    Dripline
)


pbuilder_executables( sources PACKAGE_LIBS PUBLIC_EXT_LIBS PRIVATE_EXT_LIBS  )
