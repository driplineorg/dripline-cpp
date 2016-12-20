# dripline-cpp
C++ Implementation of the [Dripline](http://www.project8.org/dripline) framework 

## Requirements

* CMake 3.0
* C++11
* Boost 1.46
* rabbitmqc

##Useful Classes
* message (and descendants msg_request, msg_alert, msg_info, msg_reply)
* service - The base class of all objects intending to use the Dripline protocol; useful objects that primarily will be sending messages synchronously.
* hub - Base class for objects that want to receive requests on a single queue and distribute them according to their routing key specifier.
* relayer - Base class for objects that want to send requests asynchronously.
