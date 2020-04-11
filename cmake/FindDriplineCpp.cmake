# TODO: put license information here

#[=======================================================================[.rst:
FindDriplineCpp
-------

Finds the dripline-cpp library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``DriplineCpp::Dripline``
  The dripline-cpp library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``DriplineCpp_FOUND``
  True if the system has the dripline-cpp library.
``DriplineCpp_VERSION``
  The version of the dripline-cpp library which was found.
``DriplineCpp_INCLUDE_DIRS``
  Include directories needed to use dripline-cpp.
``DriplineCpp_LIBRARIES``
  Libraries needed to link to dripline-cpp.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``DriplineCpp_INCLUDE_DIR``
  The directory containing ``dripline_fwd.hh``.
``DriplineCpp_LIBRARY``
  The path to the dripline-cpp library.

#]=======================================================================]

find_path( DriplineCpp_INCLUDE_DIR
  NAMES dripline_fwd.hh
  PATH_SUFFIXES Dripline
)

find_library(DriplineCpp_LIBRARY
  NAMES Dripline
)

#set(Foo_VERSION ${PC_Foo_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Foo
  FOUND_VAR DriplineCpp_FOUND
  REQUIRED_VARS
    DriplineCpp_LIBRARY
    DriplineCpp_INCLUDE_DIR
  VERSION_VAR DriplineCpp_VERSION
)

if(DriplineCpp_FOUND)
  set(DriplineCpp_LIBRARIES ${DriplineCpp_LIBRARY})
  set(DriplineCpp_INCLUDE_DIRS ${DriplineCpp_INCLUDE_DIR})
endif()

#if(Foo_FOUND AND NOT TARGET Foo::Foo)
#  add_library(Foo::Foo UNKNOWN IMPORTED)
#  set_target_properties(Foo::Foo PROPERTIES
#    IMPORTED_LOCATION "${Foo_LIBRARY}"
#    INTERFACE_COMPILE_OPTIONS "${PC_Foo_CFLAGS_OTHER}"
#    INTERFACE_INCLUDE_DIRECTORIES "${Foo_INCLUDE_DIR}"
#  )
#endif()
