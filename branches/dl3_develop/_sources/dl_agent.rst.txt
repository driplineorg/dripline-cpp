.. _dl-agent:

==============
Dripline Agent
==============

Dripline includes an executable for sending dripline messages: ``dl-agent``.


Use
===

  ``> dl-agent [options] subcommand routing_key [value] [keyword arguments]``

The subcommand and routing key are required.

Commonly-used options will include the broker (``-b,--broker``, if not specified in the 
authentication file), the specifier (``-s,--specifier``), 
and the values (``-v,--values``, or positional after the routing key)

Options
-------

::

  -h,--help                     Print this help message and exit
  -c,--config TEXT:FILE         Config file filename
  --verbosity UINT              Global logger verosity
  -V,--version                  Print the version message and exit
  -b,--broker TEXT              Set the dripline broker address
  -p,--port UINT                Set the port for communication with the dripline broker
  --auth-file TEXT              Set the authentication file path
  --requests-exchange TEXT      Set the name of the requests exchange
  --alerts-exchange TEXT        Set the name of the alerts exchange
  --max-payload UINT            Set the maximum payload size (in bytes)
  --loop-timeout-msdripline.loop-timeout-ms UINT
  --message-wait-msdripline.message-wait-ms UINT
  --heartbeat-routing-key TEXT  Set the first token of heartbeat routing keys: [token].[origin]
  --heartbeat-interval-s UINT   Set the interval between heartbeats in s
  -s,--specifier TEXT           Set the specifier
  -P,--payload TEXT ...         Add values to the payload
  -v,--values TEXT ...          Add ordered values
  -t,--timeout UINT             Set the timeout for waiting for a reply (seconds)
  -k,--lockout-key TEXT         Set the lockout key to send with the message (for sending requests only)
  --suppress-output           
  --json-print                
  --pretty-print              
  --return-code UINT            Set the return code sent (for sending replies only)
  --return-msg TEXT             Set the return message sent (for sending replies only)
  --dry-run-msg               

Subcommands
-----------

The five dripline subcommands correspond to the three request operations, 
and two other message types (reply and alert):

* ``get``    Send an OP_GET request
* ``set``    Send an OP_SET request
* ``cmd``    Send an OP_CMD request
* ``reply``  Send a reply
* ``alert``  Send an alert

Routing Key
-----------

The routing key specifies the destination for the message and is required for each subcommand.

Note that the routing key does not appear in the top-level help, ``dl-agent -h``, because it's 
technically an option of the subcommand.  It will show up if using ``dl-agent subcommand -h``.

Values and Keyword Arguments
----------------------------

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

See :ref:`Authentication<authentication>` for information on how to specify the broker and authentication information.
