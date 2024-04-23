#! /usr/bin/env bats

@test "ping my_store" {
    dl-agent -vv --auth-file /auths.json cmd simple -s ping
}

@test "get peaches" {
    dl-agent -vv --auth-file /auths.json get simple
}

@test "set peaches" {
    dl-agent -vv --auth-file /auths.json set simple 500
}