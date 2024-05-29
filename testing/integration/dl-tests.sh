#! /usr/bin/env bats

@test "ping simple" {
    dl-agent -vv -b rabbit-broker --auth-file /auths.json cmd simple -s ping
}

@test "get simple" {
    dl-agent -vv -b rabbit-broker --auth-file /auths.json get simple
}

@test "set simple" {
    dl-agent -vv -b rabbit-broker --auth-file /auths.json set simple 500
}
