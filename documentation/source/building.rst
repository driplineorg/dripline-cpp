.. _building:

=================
Building Dripline
=================

Dripline is typically built either in the :ref:`Standalone<standalone-option>` configuration,
or as a :ref:`Submodule<submodule-option>`.

For either build option, the :ref:`dependencies<dependencies>` must be installed first,
and a number of :ref:`build options<build-options>` can be used to customize the build.

.. _dependencies:

Dependencies
============

Most of the dependencies are available as system packages:

* Debian/Ubuntu::

    apt-get install build-essential cmake ninja-build git pkg-config \
        libyaml-cpp-dev rapidjson-dev libboost-dev libboost-filesystem-dev \
        libboost-system-dev libboost-chrono-dev libssl-dev zlib1g-dev libzstd-dev

* macOS (Homebrew)::

    brew install cmake ninja yaml-cpp rapidjson boost openssl@3 zstd

Two dependencies are not packaged anywhere: `rmqcpp <https://github.com/bloomberg/rmqcpp>`_,
the AMQP client library, and the `BDE <https://github.com/bloomberg/bde>`_ libraries that
rmqcpp is built on.
Both are built from pinned sources by the ``install_dependencies.sh`` script in the top-level
directory of the repository::

    ./install_dependencies.sh --prefix=/usr/local

Run ``./install_dependencies.sh --help`` for the full set of options, which include the
install prefix, the number of parallel build jobs, and the dependency versions.
The script builds in a temporary directory and removes it when it is finished.
Note that building BDE takes several minutes.

If the install prefix is not one that CMake searches by default, pass it on to the
dripline-cpp build with ``CMAKE_PREFIX_PATH``::

    ./install_dependencies.sh --prefix=${HOME}/dripline-deps
    cmake -DCMAKE_PREFIX_PATH=${HOME}/dripline-deps ..

Note that vcpkg is neither used nor required.

.. note::
   rmqcpp uses ``find_package(zstd CONFIG)``, which needs a zstd CMake package config file.
   A few distributions ship zstd without one (Ubuntu 22.04, for example).
   On those, run the script with ``--no-compression``, which builds rmqcpp without its zstd
   compression support.

The ``Dockerfile`` in the repository performs all of these steps, and is the most complete
reference for setting up a build environment.

.. _build-options:

Build Options
=============

Dripline build options include:

.. glossary::

   ``Dripline_BUILD_EXAMPLES`` (BOOL)
      if ON, builds the example applications

   ``Dripline_BUILD_PYTHON`` (BOOL)
      if ON, builds utilities used by the Python wrapping (Dripline python wrapping is done in Dripline-python)

   ``Dripline_ENABLE_EXECUTABLES`` (BOOL)
      if ON, builds the Dripline executables

   ``Dripline_ENABLE_TESTING`` (BOOL)
      if ON, builds the testing application

   ``Dripline_MAX_PAYLOAD_SIZE`` (INT)
      Maximum payload size in bytes

   ``Dripline_OFFLINE`` (BOOL)
      if ON, builds the library in offline mode (no broker connection required).
      In this mode, sending a message throws the message object rather than transmitting it,
      which is useful for unit testing.
      See :ref:`Offline & No-Connection <offline>` for more details.

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

.. _standalone-option:

Standalone Option
=================

In standalone mode, one compiles only the dripline-cpp repository and its dependencies.
This option is best for testing, providing access to command-line tools, or when a derrived software product uses dripline-cpp as an external dependency.
This is the approach used in the Dockerfile, which is included in the dripline-cpp repository and used to automatically build dripline-cpp container images.
Dripline-python's recommended build also expects dripline-cpp as an external dependency.
The steps are:

1. Clone the repository::

    git clone https://github.com/driplineorg/dripline-cpp --recurse-submodules

2. Install the :ref:`dependencies <dependencies>`, including rmqcpp and BDE::

    cd dripline-cpp
    ./install_dependencies.sh

3. Create a build directory::

    mkdir build
    cd build

4. Run CMake::

    cmake ..

5. Build and install::

    make install


.. _submodule-option:

Submodule Option
================

The submodule build configuration is for when the dripline-cpp is being include as a git submodule in some derived package's repository.
This option is best when you want to track and pin the exact dripline-cpp version to be used in another project and you want to build them together using cmake.
The steps for this mode are:

1. Go to the top-level directory of the parent pacakge in which you want to include dripline-cpp::

    cd /path/to/my_package

2. Add the submodule::

    git submodule add https://github.com/driplineorg/dripline-cpp [desired path]

3. Set the desired dripline build options from the parent package's CMakeLists.txt file.
