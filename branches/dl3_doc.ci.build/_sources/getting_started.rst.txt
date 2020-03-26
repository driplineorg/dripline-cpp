===============
Getting Started
===============

What's Included
===============

* dl-agent: An executable that allows you to send Dripline messages
* dl-mon: An executable that monitors messages on the mesh
* dripline-cpp library: Dripline API that can be used to send and receive messages

Requirements
============

* CMake 3.1
* C++11
* Boost 1.46
* rabbitmqc

Included Packages
=================

* SimpleAmqpClient

How to Install
==============

There are two main ways to use dripline-cpp:

1. As a standalone package.

2. As a submodule of another package.  

Standalone Installation
-----------------------

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

Submodule Installation
----------------------

1. Go to the top-level directory of the repo in which you want to include dripline-cpp::

    cd /path/to/my_package

1. Add the submodule::

    git submodule add https://github.com/driplineorg/dripline-cpp [desired path]
