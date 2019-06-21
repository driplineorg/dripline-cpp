===========================
Offline & No-Connection Use
===========================

Dripline-cpp can be used in two ways without connecting to a broker:  Offline is a build-time option, and no-connection use is a build- or run-time option.

Offline
-------

This mode is a build-time option that is typically used for unit-testing purposes.

In this mode:

* Sending a message results in the message object being thrown (in the C++ sense).  This feature is useful for testing the various send-message options.

* Receiving messages is currently disabled, as is any other function that uses the SimpleAMQP library API.  Such methods will respond with error conditions.  Some message-receive functionality could be tested using the Offline mode in the future, e.g. by manually submitting a SimpleAMQP envelope object.

Offline mode is selected by building the library with the ``Dripline_OFFLINE`` flag enabled in CMake.

No-Connection
-------------

Some dripline classes have flags in the constructors that allow them to be used without making a connection to a broker.  This can be found in ``service`` and ``hub``.

Depending on how it's used, this could be done as either a build-time or a run-time option.

For an object in no-connection mode, methods that use the SimpleAMQP API will respond with error conditions.

This feature is useful, for example, when a user wants to be able to handle message objects that are not received from a broker (e.g. they're created manually).
