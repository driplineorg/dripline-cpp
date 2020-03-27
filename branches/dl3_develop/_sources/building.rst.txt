.. _build:
=================
Building Dripline
=================

Dripline is typically built either in the :ref:`Standalone<standalone-option>` configuration, 
or in the :ref:`Submodule<submodule-option>`.

For either build option, a number of :ref:`build options<build-options>` can be used to customize the build.

.. _build-options:
Build Options
=============

Dripline build options include:

* ``Dripline_BUILD_EXAMPLES``: if ON, builds the example applications
* ``Dripline_BUILD_PYTHON``: if ON, builds utilities used by the Python wrapping
* ``Dripline_ENABLE_EXECUTABLES``: if ON, builds the Dripline executables
* ``Dripline_ENABLE_TESTING``: if ON, builds the testing application
* ``Dripline_MAX_PAYLOAD_SIZE``: Maximum payload size in bytes
* ``Dripline_PYTHON_THROW_REPLY_KEYWORD``: keyword used by the python wrapping that indicates that the thrown object is a reply message to be sent

From the built-in CMake options, users may be interested in adjusting the build type 
to control the level of compiler optimizationa and terminal output.  
The CMake option is ``CMAKE_BUILD_TYPE``, and the options are:

* ``Debug``: prints info and debug messages, and can print trace messages; low compiler optimization; debugging symbols included
* ``Release``: only prints warning and error messages; highly optimized, and no debugging symbols included

You can additionally define preprocessor macros that are used directly in the code:

* ``DRIPLINE_AUTH_FILE``: specifies the location and filename of the default authentication file

.. _standalone-option:
Standalone Option 
=================

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

1. Go to the top-level directory of the parent pacakge in which you want to include dripline-cpp::

    cd /path/to/my_package

2. Add the submodule::

    git submodule add https://github.com/driplineorg/dripline-cpp [desired path]

3. Set the desired dripline build options from the parent package's CMakeLists.txt file.
