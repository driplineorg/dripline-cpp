=============
Configuration
=============

An application that uses the Scarab application framework, including ``dl-agent`` and ``dl-mon``, 
will have a global dictionary-like set of configuration parameters.  
The parameters can generally be divided into two sets:

    1. Dripline-mesh parameters, which describe the specifications of the mesh that is being used, and 
    2. Application parameters, which are particular to the application being run (e.g. ``dl-agent`` or a service)

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

    dripline_mesh:
        broker: localhost
        broker_port: 5672
        requests_exchange: requests
        alerts_exchange: alerts
        max_payload_size: DL_MAX_PAYLOAD_SIZE
        loop_timeout_ms: 1000
        message_wait_ms: 1000
        heartbeat_routing_key: heartbeat
        hearteat_interval_s: 60

.. _default-mesh-yaml:

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

.. _authentication:

Authentication
==============

Authentication information is handled separately from other configuation parameters since it's 
usually sensitive information that shouldn't be exposed.  The authentication information is 
specified through a combination of application-specific defaults and user overrides. 

In the dripline context, the main authentication item is for the RabbitMQ broker: a username 
and password are required.  Other authentication items may also be present: e.g. for database 
access or posting messages to something like Slack.

In order of precedence, with items lower on the list overriding those higher on the list, the 
sources of authentication information are:

    1. Application defaults.  For dripline, the default username and password are ``guest`` and ``guest``, 
    which match the defaults used by the RabbitMQ broker.

    2. Environment variables.  By default dripline uses ``DRIPLINE_USERNAME`` and ``DRIPLINE_PASSWORD`` to 
    set the username and password for sending messages to the broker, respectively.  The user can change 
    the variables used at runtime.  If the variable(s) are present, their values will be used; otherwise 
    they will be ignored.

    3. A user-supplied file.  A file can be provided that contains exactly the item in question.  This is most 
    often used for passwords.  Some deployment methods use the concept of a "secrets file" that can be used to 
    provide sensitive information like a password.  The file should contain exactly the value desired for the 
    particular authentication parameter (e.g. watch out for unintentional new lines at the end of a file).   
    There is no default setting for this -- if the user does not supply a filename, no action is taken.

    4. A user-supplied value.  An authentication item can be supplied directly, overriding any other settings.  
    Be aware that this can put the value of an item into one's CLI history or otherwise expose it, which 
    can be problematic for passwords.  There is no default setting for this -- if the user does not supply a value, 
    no action is taken.

Specifying Parameters
=====================

The configuration process takes place in five stages:

    1. The default parameters are used to form the primary configuration dictionary.  If a dripline mesh 
    configuration file exists in the user's home directory (i.e. ``$HOME/.dripline_mesh.yaml``), values 
    present in that file are merged into the hard-coded defaults.

    2. If specified, a configuration file is parsed and merged with the stage-1 configuration.

    3. Any keyword non-option arguments (i.e. ``key=value``) given on the command line are 
    merged with the stage-2 configuration.

    4. Any command-line options (i.e. ``--parameter value``) are merged with the stage-3 configuration.

    5. If any parameters have been specified to include environment variable values, the variables are checked and 
    the values are inserted into the parameter values.

After stage five, the primary configuration dictionary is passed to the application.

Configuration File
------------------

A configuration file, written in YAML or JSON, can be provided on the command-line.  This file can specify any parameters 
that the user wants to configure via the file.  Parameters not included will be set to their default values.

Keyword Arguments
-----------------

A keyword argument can modify any existing parameter value.  The format for the argument is ``key=value``.

The ``key`` is used to address the particular parameter in the configuration hierarchy.  If the configuration 
is viewed as a nested set of array-like and dictionary-like structures, any value in that structure can be 
addressed with the following syntax: a combination of strings and integers, each of which indicates 
a position in the nested dictionaries (string keys) and arrays (integer keys), separated by ``.``.  
For example, given this configuration:

.. code-block:: YAML

    mercury:
      moons: []
      surface_temp: 167
    venus:
      moons: []
      surface_temp: 464
    earth:
      moons:
        - The moon
      surface_temp: 18
    mars:
      moons:
        - Phobos
        - Deimos
      surface_temp: -65

You could fix the average temperature on early with ``earth.surface_temp=15`` or change the name 
of Mars' second moon with ``mars.moons.1=moony``.

Command-Line Options
--------------------

As a general principle, each application specifies the set of command-line (CL) options that it will use.  
There is a default set of CL options that all dripline executables include:

::

    -h,--help                     Print this help message and exit
    -c,--config TEXT:FILE         Config file filename
    --config-encoding TEXT        Config file encoding
    -v,--verbose                  Increase verbosity
    -q,--quiet                    Decrease verbosity
    -V,--version                  Print the version message and exit
    -u,--username TEXT            Specify the username for the rabbitmq broker
    --password TEXT               Specify a password for the rabbitmq broker -- NOTE: this will be plain text on the command line and may end up in your command history!
    --password-file TEXT          Specify a file (e.g. a secrets file) to be read in as the rabbitmq broker password
    --auth-file TEXT              Set the authentication file path
    -b,--broker TEXT              Set the dripline broker address
    -p,--port UINT                Set the port for communication with the dripline broker
    --requests-exchange TEXT      Set the name of the requests exchange
    --alerts-exchange TEXT        Set the name of the alerts exchange
    --max-payload UINT            Set the maximum payload size (in bytes)
    --heartbeat-routing-key TEXT  Set the first token of heartbeat routing keys: [token].[origin]

Specific applications will add further options.  For example, ``dl-agent`` adds options having to do with 
sending messages, and ``dl-mon`` adds options having to do with monitoring messages.

Environment Variables
---------------------

Environment variables can be used to substitute values into configuration parameters.  The syntax used in 
the configuration parameter value is: ``ENV{<variable>}``.  That syntax needs to be inserted into or as a 
configuration parameter value in one of the four previous configuration stages.

If an environment variable is specified in the configuration but the variable does not exist in 
the environment, an exception will be thrown.

Here's an example configuration, shown in YAML format, where environment variable subsitution is requested:

.. code-block:: YAML

    dripline_mesh:
      broker: ENV{DL_PREFIX}-broker
      broker-port: ENV{DL_PORT}

In this case the user wants a customized broker address specified at runtime by the contents of the ``DL_PREFIX`` 
environment variable, and they want to specify the port with ``DL_PORT``.

Recommended Setup
=================

The above sections describe many ways in which Dripline applications can be configured.  
For convenience to the user and ease of maintenance, we recommend the following setup:

    1. Provide the default mesh information in a ``.dripline_mesh.yaml`` file in the user's home directory.  
    At a minimum, include the broker address in that file.
    2. For authentication information:

        A. For manual interactive use (e.g. using ``dl-agent``), supply the RabbitMQ login details, and 
        the login details for any other applications (e.g. database) with environment variables.

        B. For deployed use (e.g. Docker Swarm or Kubernetes running services) either use environment variables 
        or secrets files.

Examples
========

The dripline-cpp integration tests provide a variety of examples of how to configure dripline-cpp applications.  
These are found in the source directory ``testing/integration``.  
You'll find several services started in different ways in ``docker-compose.yaml``, 
and a number of ``dl-agent`` commands configured differently in ``dl-tests.sh``.
    