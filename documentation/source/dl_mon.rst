.. _dl-mon:

================
Dripline Monitor
================

Dripline includes an executable for monitoring the messages being passed around on a mesh: ``dl-mon``.

.. TODO sphinx supports autodoc for the CLI tools. We should consider replacing the following code blocks with parsed CLI output from `--help` in the future (if we're building in an environment where dripline-cpp is installed).

Use
===

  ``> dl-mon [options] [keyword arguments]``

The user should specify routing keys to be monitored on the requests or alerts exchange using either or both of the ``-r,--requests`` and ``-a,--alerts`` options.
Standard RabbitMQ wildcard rules apply.

Options
-------

::

  -h,--help                   Print this help message and exit
  -c,--config TEXT:FILE       Config file filename
  -v,--verbose                Increase verbosity
  -q,--quiet                  Decrease verbosity
  -V,--version                Print the version message and exit
  -b,--broker TEXT            Set the dripline broker address
  -p,--port UINT              Set the port for communication with the dripline broker
  --auth-file TEXT            Set the authentication file path
  --requests-exchange TEXT    Set the name of the requests exchange
  --alerts-exchange TEXT      Set the name of the alerts exchange
  --max-payload UINT          Set the maximum payload size (in bytes)
  --loop-timeout-msdripline.loop-timeout-ms UINT
  --message-wait-msdripline.message-wait-ms UINT
  --heartbeat-routing-key TEXT
                              Set the first token of heartbeat routing keys: [token].[origin]
  --heartbeat-interval-s UINT Set the interval between heartbeats in s
  -r,--requests TEXT ...      Assign keys for binding to the requests exchange
  -a,--alerts TEXT ...        Assign keys for binding to the alerts exchange
  --json-print                
  --pretty-print              

Keyword Arguments
-----------------

Arguments in the form [key]=[value] will be assumed to be keyword arguments that can be used to set configuration parameters.

The "key" portion of a keyword argument is an address that can specify both node and array locations.
For example, ``my.value.0=10`` add/set this in the configuration::

    my:
      value:
        - 10

Authentication
==============

Communication with the RabbitMQ broker requires user/password authentication. 

See :ref:`Authentication<authentication>` for information on how to specify the broker and authentication information.
