.. _authentication:

==============
Authentication
==============

Broker Information
==================

The basic information needed for communicating with the broker is the broker address and the port.  
These can be specified either in the application configuration file or as command-line arguments.

Configuration file (YAML)::

    broker: my.broker
    broker-port: 5362

Command-line arguments::

    --broker my.broker
    --port 5762

If your broker is using the default port (5762), it doesn't need to be separately specified when running 
any dripline applications.

RabbitMQ Authentication
-----------------------

Authentication for communication with the RabbitMQ broker is performed with a username/password pair.  

Authentication File
===================

The authentication information is typically stored in an authentication file (JSON format) 
on the machine running the dripline application.  
The authentication file can also store the broker address and port, 
though the broker information can also be overridden with an application configuration file and 
command-line arguments.

An authentication file for dripline can look like this (JSON)::

    {
        "amqp": {
            "username": "a_user",
            "password": "123456"
            "broker": "my.broker"
            "broker-port": 5762
        }
    }

The username and password are required, but the ``broker`` and ``broker-port`` are optional, 
as they may be specified in the application configuraiton file or command-line arguments.

Specifying the File
-------------------

The default path can be set at build time by defining ``DRIPLINE_AUTH_FILE`` before building dripline-cpp 
in a derived project.  If that macro is not defined, then there is no default path.

The authentication file is specified in an application configuration as:

    amqp:
        auth-file: [filename]

At runtime the authentication file path can be modified by any of the normal ways, plus the 
command-line argument:

    --auth-file [file]
