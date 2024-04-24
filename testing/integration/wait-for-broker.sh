#! /bin/bash

# TODO: remove this install of curl when the dl-py image has curl
#apt-get update && apt-get install -y curl

# Be sure we have curl
#which curl
#if [ $? ]; then 
#    apt-get update && apt-get install -y curl
#    echo "curl is not installed"
#    exit 0
#fi

# we keep track of the time to make sure this doesn't run forever in case there's a problem reaching the broker
start=$SECONDS
until curl -u dripline:dripline http://rabbit-broker:15672/api/overview &> /dev/null
do
    echo "Broker is not ready"
    sleep 1
    duration=$(( SECONDS - start ))
    # 60 seconds should be plenty of time to get the broker started if it's going to work
    if [ $duration -ge 20 ]; then 
        echo "Wait for broker timed out"
        exit 0
    fi
done
echo "############## BROKER IS READY ################"
exit 0