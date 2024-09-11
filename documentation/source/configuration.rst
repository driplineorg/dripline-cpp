=============
Configuration
=============

An application that uses the Scarab application framework, including ``dl-agent`` and ``dl-mon``, 
will have a global dictionary-like set of configuration parameters.  
In this documentation, configurations will be indicated using YAML, and the mapping collection.  
In the source code, this is implemented as a ``scarab::param_node``. 

Dripline Parameters
===================

All dripline-based applications must have a common set of mesh configuration parameters.  
These parameters are grouped in the configuration with the key ``dripline_mesh``. 

Explanation of Parameters
-------------------------

Here is the possible set of parameters for the ``dripline_mesh`` block, with a short description of each one:

.. code-block:: YAML

    dripline_mesh:
        broker: (string) address of the broker
        broker_port: (unsigned int) port for exchanging AMQP messages with the broker
        requests_exchange: (string) name of the requests exchange
        alerts_exchange: (string) name of the alerts exchange
        max_payload_size: (unsigned int) maximum payload size in bytes
        loop_timeout_ms: (unsigned int) time used in loops for checking for application shutdown in milliseconds
        message_wait_ms: (unsigned int) timeout for waiting for a message in milliseconds
        heartbeat_routing_key: (string) routing key for sending and receiving heartbeat messages
        hearteat_interval_s: (unsigned int) interval for sending heartbeats in seconds
        return_codes:
          - name: (string) return-code name (must be unique)
            value: (unsigned int) return-code value (must be unique)
            description: (string) human-readable description of what the return-code means
          - ...

Default Parameters
------------------

The defaults for all of these parameters are given in the class ``dripline_config``:

.. code-block:: YAML

    dripline:
        broker: localhost
        broker_port: 5672
        requests_exchange: requests
        alerts_exchange: alerts
        max_payload_size: DL_MAX_PAYLOAD_SIZE
        loop_timeout_ms: 1000
        message_wait_ms: 1000
        heartbeat_routing_key: heartbeat
        hearteat_interval_s: 60

Default parameters can be modified with a YAML file placed in the user's home directory.  
Specifically, the file should be ``$HOME/.dripline_mesh.yaml``.  A common application of this 
feature is to set the broker address.  To set the broker address to ``my-broker``, 
the ``.dripline_mesh.yaml`` file should consist of:

.. code-block:: YAML
    broker: my-broker

Note that the default ``max-payload-size`` are defined by preprocessor macros that 
should be defined before building dripline-cpp.  ``DL_MAX_PAYLOAD_SIZE`` has a default 
value within dripline-cpp.

The default set of return codes are specified in ``return_codes.cc``.  There are no default return codes 
in the class ``dripline_config``.

The ``dripline_config.hh`` header define the function ``add_dripline_options()`` 
that will add the common dripline command-line options within the scarab application framework.

Application Parameters
======================

Typically an application will have its own configuration parameters that will include 
the ``dripline_mesh`` block.  For example, ``dl-agent`` is configured from ``agent_config``:

.. code-block:: YAML

    timeout: 10
    dripline_mesh:
        broker: localhost
        . . .

The application configuration parameters can be arbitrarily complicated, 
according to the scarab application framework, 
and for dripline purposes they just need to contain the ``dripline_mesh`` block.

Specifying Parameters
=====================

The configuration process takes place in four stages:

    1. The default parameters are used to form the master configuration dictionary.

    2. If specified, a configuration file is parsed and merged with the stage-1 configuration.

    3. Any keyword non-option arguments (i.e. ``key=value``) given on the command line are 
    merged with the stage-2 configuration.

    4. Any command-line options (i.e. ``--parameter value``) are merged with the stage-3 configuration.

After stage four, the master configuration dictionary is passed to the application.

