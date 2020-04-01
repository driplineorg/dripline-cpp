=============
Configuration
=============

An application that uses the Scarab application framework, including ``dl-agent`` and ``dl-mon``, 
will have a global dictionary-like set of configuration parameters.  
In this documentation, configurations will be indicated using YAML, and the mapping collection.  
In the source code, this is implemented as a ``scarab::param_node``. 

Dripline Parameters
===================

All dripline-based applications must have a common set of configuration parameters.  
These parameters are indicated by the key ``amqp``.  
The defaults for all of these parameters are given in the class ``dripline_config``:

    amqp:
        auth-file: DRIPLINE_AUTH_FILE
        requests-exchange: requests
        alerts-exchange: alerts
        max-payload-size: DL_MAX_PAYLOAD_SIZE
        loop-timeout-ms: 1000
        message-wait-ms: 1000
        heartbeat-routing-key: heartbeat
        hearteat-interval-s: 60

Note that ``auth-file`` and ``max-payload-size`` are defined by preprocessor macros that 
should be defined before building dripline-cpp.  ``DL_MAX_PAYLOAD_SIZE`` has a default 
value, but ``DRIPLINE_AUTH_FILE`` must be defined by the client code/user, and 
must be a valid file at runtime for the default to be placed in the ``amqp``.

The ``dripline_config.hh`` header define the function ``add_dripline_options()`` 
that will add the common dripline command-line options within the scarab application framework.

Application Parameters
======================

Typically an application will have its own configuration parameters that will include 
the ``amqp`` block.  For example, ``dl-agent`` is configured from ``agent_config``:

    timeout: 10
    amqp:
        auth-file: DRIPLINE_AUTH_FILE
        . . .

The application configuration parameters can be arbitrarily complicated, 
according to the scarab application framework, 
and for dripline purposes they just need to contain the ``amqp`` framework.

Specifying Parameters
=====================

The configuration process takes place in four stages:

    1. The default parameters are used to form the master configuration dictionary.

    2. If specified, a configuration file is parsed and merged with the stage-1 configuration.

    3. Any keyword non-option arguments (i.e. ``key=value``) given on the command line are 
    merged with the stage-2 configuration.

    4. Any command-line options (i.e. ``--parameter value``) are merged with the stage-3 configuration.

After stage four, the master configuration dictionary is passed to the application.

Broker and Broker Port
======================

In a dripline application, a few parameters can be specified from the :ref:`authentication  <authentication>` file:

    broker: [broker URL]
    broker-port: [broker port]

The values given in the authentications file form the default on top of which the master-config is applied.  
In other words, if ``broker`` or ``broker-port`` are specified in the master configuration, they override 
the values from the authentications file.  For that reason, ``dripline_config`` does not specify either of 
those values.

At the source-code level, clients of ``core`` can override ``broker`` and ``broker-port`` by specifying 
parameters of the constructor.  There are a few other arguments that can be overridden in this way, as well.