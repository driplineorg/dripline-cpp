# CMakeLists.txt for dripline-cpp/executables
# Author: N. Oblath
# Created: Jan 5, 2016

##########

set( lib_dependencies 
    Dripline
)
set( programs
    dl-agent
)
pbuilder_executables( programs lib_dependencies )
