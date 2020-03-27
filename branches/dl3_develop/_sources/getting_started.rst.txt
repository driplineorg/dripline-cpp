.. _getting_started:
===============
Getting Started
===============

What's Included
===============

* ``dl-agent``: An executable that allows you to send Dripline messages
* ``dl-mon``: An executable that monitors messages on the mesh
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


Getting Dripline-cpp
====================

The Dripline-cpp repository is available on `GitHub <https://driplineorg.github.com/dripline-cpp>`_.

Dripline-cpp can either be built manually, or used from a Docker container.

For manual installation instructions, see :ref:`building:build`.

Pre-built docker containers containing a dripline-cpp installation are available on `Docker Hub <https://hub.docker.com/repository/docker/driplineorg/dripline-cpp>`_.

Images are automatically built for each tagged release, and for the ``master`` and ``develop`` branches.

Users can also build their own images using the provided Dockerfile or by customizing their own.


Using Dripline-cpp
================== 

See :ref:`agent` and :ref:`monitor` for instructions on using the included applications.

See :ref:`library` for instructions on using the Dripline C++ API.


Setting Up a Mesh
=================

Instructions and examples for setting up a dripline mesh can be found 
from the main `documentation site <https://driplineorg.github.io>`_.
