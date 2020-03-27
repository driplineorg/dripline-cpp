.. _monitor:
================
Dripline Monitor
================

Dripline includes an executable for monitoring the messages being passed around on a mesh: ``dl-mon``.


Use
===

  ``> dl-mon [options] [keyword arguments]``

The user should specify routing keys to be monitored using either or both 
of the ``-r,--requests`` and ``-a,--alerts`` options.  Standard RabbitMQ wildcard rules apply.

Options
-------

::

  -h,--help                   Print this help message and exit
  -c,--config TEXT:FILE       Config file filename
  --verbosity UINT            Global logger verosity
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

Positional arguments can be used to add to the values array in the payload and to other parts of the payload.

Arguments in the form [key]=[value] will be assumed to be keyword arguments.
Other arguments will be assumed to be entries in the values array.

The "key" portion of a keyword argument is an address that can specify both node and array locations.
For example, ``my.value.0=10`` would add this to the payload::

    my:
      value:
        - 10


Authentication
==============

Communication with the RabbitMQ broker requires the broker address and port, and user/password authentication. 

See :ref:`authentication` for information on how to specify the broker and authentication information.
