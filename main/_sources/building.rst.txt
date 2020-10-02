.. _building:

=================
Building Dripline
=================

Dripline is typically built either in the :ref:`Standalone<standalone-option>` configuration, 
or as a :ref:`Submodule<submodule-option>`.

For either build option, a number of :ref:`build options<build-options>` can be used to customize the build.

.. _build-options:

Build Options
=============

Dripline build options include:

.. glossary::

   ``Dripline_BUILD_EXAMPLES`` (BOOL)
      if ON, builds the example applications

   ``Dripline_BUILD_EXAMPLES`` (BOOL)
      if ON, builds the example applications

   ``Dripline_BUILD_PYTHON`` (BOOL)
      if ON, builds utilities used by the Python wrapping

   ``Dripline_ENABLE_EXECUTABLES`` (BOOL)
      if ON, builds the Dripline executables

   ``Dripline_ENABLE_TESTING`` (BOOL)
      if ON, builds the testing application

   ``Dripline_MAX_PAYLOAD_SIZE`` (INT)
      Maximum payload size in bytes

   ``Dripline_PYTHON_THROW_REPLY_KEYWORD``
      keyword used by the python wrapping that indicates that the thrown object is a reply message to be sent


From the built-in CMake options, users may be interested in adjusting the build type
to control the level of compiler optimization and terminal output.
The CMake option and possible values are:

.. glossary::

   ``CMAKE_BUILD_TYPE`` (STRING: [Debug || Release])
      ``Debug``
         prints info and debug messages, and can print trace messages; low compiler optimization; debugging symbols included
      ``Release``
         only prints warning and error messages; highly optimized, and no debugging symbols included

You can additionally define preprocessor macros that are used directly in the code:

.. glossary::

   ``DRIPLINE_AUTH_FILE`` (PATH)
      specifies the location and filename of the default authentication file


.. _standalone-option:

Standalone Option
=================

In standalone mode, one compiles only the dripline-cpp repository and its dependencies.
This option is best for testing, providing access to commandline tools, or when a derrived software product uses dripline-cpp as an external dependency.
This is the approach used in the Dockerfile, which is included in the dripline-cpp repository and used to automatically build dripline-cpp container images.
The steps are:

1. Clone the repository::

    git clone https://github.com/driplineorg/dripline-cpp --recurse-submodules

2. Create a build directory::

    cd dripline-cpp
    mkdir build
    cd build

3. Run CMake::

    cmake ..

4. Build and install::

    make install


.. _submodule-option:

Submodule Option
================

The submodule build configuration is for when the dripline-cpp is being include as a git submodule in some derived package's repository.
This option is best when you want to track and pin the exact dripline-cpp version to be used in another project and you want to build them together using cmake.
This is the approach used in the dripline-python package, which builds a python library on top of dripline-cpp.
The steps for this mode are:

1. Go to the top-level directory of the parent pacakge in which you want to include dripline-cpp::

    cd /path/to/my_package

2. Add the submodule::

    git submodule add https://github.com/driplineorg/dripline-cpp [desired path]

3. Set the desired dripline build options from the parent package's CMakeLists.txt file.
