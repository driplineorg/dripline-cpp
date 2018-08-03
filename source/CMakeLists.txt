#Copyright 2016 Noah S. Oblath
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

# CMakeLists.txt for dripline/cpp/source
# Author: N. Oblath
# Created: Jan 5, 2016

##########

set( dir ${CMAKE_CURRENT_SOURCE_DIR} )

set( dripline_HEADERS
    ${dir}/agent.hh
    ${dir}/agent_config.hh
    ${dir}/amqp.hh
    ${dir}/core.hh
    ${dir}/dripline_api.hh
    ${dir}/dripline_constants.hh
    ${dir}/dripline_error.hh
    ${dir}/dripline_version.hh
    ${dir}/hub.hh
    ${dir}/message.hh
    ${dir}/relayer.hh
    ${dir}/reply_package.hh
    ${dir}/routing_key_specifier.hh
    ${dir}/service.hh
    ${dir}/uuid.hh
)

set( dripline_SOURCES
    ${dir}/agent.cc
    ${dir}/agent_config.cc
    ${dir}/core.cc
    ${dir}/dripline_constants.cc
    ${dir}/dripline_error.cc
    ${dir}/dripline_version.cc
    ${dir}/hub.cc
    ${dir}/message.cc
    ${dir}/relayer.cc
    ${dir}/reply_package.cc
    ${dir}/routing_key_specifier.cc
    ${dir}/service.cc
    ${dir}/uuid.cc
)

configure_file( ${dir}/dripline_version.cc.in ${CMAKE_CURRENT_BINARY_DIR}/dripline_version.cc )
set( dripline_SOURCES
    ${dripline_SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/dripline_version.cc
)


##########

pbuilder_library( Dripline dripline_SOURCES "" )

pbuilder_install_headers( ${dripline_HEADERS} )

if( Dripline_ENABLE_EXECUTABLES )
    set( lib_dependencies 
        Dripline
    )
    set( programs
        dl_agent
    )
    pbuilder_executables( programs lib_dependencies )
endif( Dripline_ENABLE_EXECUTABLES )
