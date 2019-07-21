==============
Dripline Agent
==============

Dripline includes an executable for sending dripline messages: ``dl_agent``.

Use
===

  ``> dl_agent [options] routing_key [specifier] subcommand [values] [keyword arguments]``

Routing Key
-----------

Provide the AMQP routing key to which you're sending your message (required).

Specifier
---------

Provide the dripline specifier to further direct your message (optional).

Subcommands
-----------

The six dripline subcommands are available as:

* ``run``    Send an OP_RUN request
* ``get``    Send an OP_GET request
* ``set``    Send an OP_SET request
* ``cmd``    Send an OP_CMD request
* ``reply``  Send a reply
* ``alert``  Send an alert

Values and Keyword Arguments
----------------------------

Any non-option arguments other than the routing key and specifier will be counted either as values or keyword arguments.

Values are arguments that are not in the form ``keyword=value``.  They are added to the ``values`` array in the payload in the order received.

Keyword arguments are arguments in the form ``keyword=value``.  They are added to the payload.  
The keyword is the address within the payload.  For example, ``my.value.0=10`` would add this to the payload::

    my:
      value:
        - 10


Keyword Arguments
-----------------

Options
-------

::

-h,--help                   Print this help message and exit
-c,--config FILE            Config file filename
--verbosity UINT            Global logger verosity
-V,--version                Print the version message and exit
-b,--broker TEXT            Set the dripline broker address
-p,--port UINT              Set the port for communication with the dripline broker
-e,--exchange TEXT          Set the exchange to send message on
-a,--auth-file TEXT         
-t,--timeout UINT           Set the timeout for waiting for a reply (seconds)
-k,--lockout-key TEXT       Set the lockout key to send with the message
--payload TEXT ...          Add values to the payload
-v,--values TEXT ...        Add ordered values
--dry-run-msg               Print the message contents and do not send
--pretty-print              Outputs nicely-formatted JSON to the terminal
--suppress-output           Normal message output is suppressed
--return-code               Set the return code to send (replies only)
--return-message            Set the return message to send (replies only)


Output
======

The ``stdout`` output of the agent will be the reply, if any, that is received from the sent message.

All other terminal output (e.g. logging messages of all types) are sent to ``stderr``.

The return code will be the integer-truncated return code of the reply (if any) divided by 100.  
If there is no reply message (by design or error), a successfully-sent message will return 0 and 
an error will return the integer-truncated client error code divided by 100.


Build Options
=============

The default agent configuration can include a default authentication file, which is expected to be in the user's home directory.  It is specified at build time with the preprocessor macro ``DRIPLINE_AUTH_FILE``.

In a CMake build of a package that uses dripline, it can be specified with:

  ``add_definitions( -DDRIPLINE_AUTH_FILE=[filename] )``

This should be included before the Dripline submodule is added.
