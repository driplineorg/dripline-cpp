==============
Dripline Agent
==============

Dripline includes an executable for sending dripline commands: ``dl_agent``.

Use
===

  ``> dl_agent [command] [options]``

Commands
--------

The four dripline commands are available as: ``run``, ``get``, ``set``, and ``cmd``.

Options
-------

* Routing key: ``rk=[routing key]`` (required)
* Value for ``set``: ``value=[a_value]`` (required for ``set``)
* Broker address: ``amqp.broker=[address]`` (default is ``localhost``)
* Broker port: ``amqp.broker-port=[port]`` (default is 5672)
* Exchange: ``amqp.exchange=[exchange]`` (default is ``requests``)
* Reply timeout: ``amqp.reply-timeout-ms=[ms]`` (default is ``10000``)
* Authentications file: ``amqp.auth-file=[filename]`` (default is none)
* Lockout key: ``lockout-key=[uuid]``
* Filename to save reply: ``save=[filename]`` (optional)
* Filename for payload: ``load=[filename]`` (optional)

Build Options
=============

The default agent configuration can include a default authentication file, which is expected to be in the user's home directory.  It is specified at build time with the preprocessor macro ``DRIPLINE_AUTH_FILE``.

In a CMake build of a package that uses dripline, it can be specified with:

  ``add_definitions( -DDRIPLINE_AUTH_FILE=[filename] )``

This should be included before the Dripline submodule is added.
