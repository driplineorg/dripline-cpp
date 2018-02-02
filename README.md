# dripline-cpp

This library is a C++ implementation of the [Dripline](http://www.project8.org/dripline) protocol. 

[![Documentation Status](https://readthedocs.org/projects/dripline-cpp/badge/?version=stable)](http://dripline-cpp.readthedocs.io/en/stable/?badge=stable)
[![Dripline Status](https://img.shields.io/badge/dripline-2.1.1-blue.svg)](https://img.shields.io/badge/dripline-2.1.1-brightgreen.svg)

## Useful Classes
* message (and descendants msg_request, msg_alert, msg_info, msg_reply)
* service - The base class of all objects intending to use the Dripline protocol; useful objects that primarily will be sending messages synchronously.
* hub - Base class for objects that want to receive requests on a single queue and distribute them according to their routing key specifier.
* relayer - Base class for objects that want to send requests asynchronously.
