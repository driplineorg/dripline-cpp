#! /bin/bash

# wait to make sure service connects
sleep 2s

dl-agent -vv -b rabbit_broker --auth-file /root/authentication.json get simple

# TODO: make a better check of the return

exit 0
