===============
Getting Started
===============

What's Included
===============

* :ref:`dl-agent <dl-agent>`: An executable that allows you to send Dripline messages
* :ref:`dl-mon <dl-mon>`: An executable that monitors messages on the mesh
* dripline-cpp library: Dripline API that can be used to send and receive messages


Requirements
============

* CMake 3.12
* C++17
* Boost 1.46
* rabbitmqc
* libyaml-cpp
* rapidjson
* quill (logging library)


Included Packages
=================

* `SimpleAmqpClient <https://github.com/project8/SimpleAmqpClient>`_ (unchanged fork of `upstream SimpleAmqpClient <https://github.com/alanxz/SimpleAmqpClient>`_)
* `scarab <https://github.com/project8/scarab>`_


Getting dripline-cpp
====================

The dripline-cpp repository is available on `GitHub <https://driplineorg.github.com/dripline-cpp>`_.

Dripline-cpp can either be built manually, or used from a Docker container.

For manual installation instructions, see :ref:`Installation <building>`.

Pre-built docker containers containing a dripline-cpp installation are available on `Docker Hub <https://hub.docker.com/repository/docker/driplineorg/dripline-cpp>`_.

Images are automatically built for each tagged release, and for the ``main`` and ``develop`` branches.

Users can also build their own images using the provided Dockerfile or by customizing their own.


Using dripline-cpp
================== 

See :ref:`Agent <dl-agent>` and :ref:`Monitor <dl-mon>` for instructions on using the included applications.

See :ref:`Library <library>` for instructions on using the Dripline C++ API.


Setting Up a Mesh
=================

Instructions and examples for setting up a dripline mesh can be found 
from the main `dripline documentation site <https://driplineorg.github.io>`_ 
and in the dripline-cpp integration tests.
