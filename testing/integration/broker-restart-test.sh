#! /usr/bin/env bats

@test "ping simple" {
    until curl -sf -u dripline:dripline http://rabbit-broker:15672/api/overview > /dev/null 2>&1; do
        echo "Broker not yet recovered; waiting..."
        sleep 2
    done
    sleep 2 # let the service reconnect
    dl-agent -vv cmd simple -s ping
}
