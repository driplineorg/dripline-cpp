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

@test "user/pass on CL" {
    dl-agent -vv -b rabbit-broker -u dripline --password dripline get simple
}

@test "user on CL/pass in file" {
    dl-agent -vvv -b rabbit-broker -u dripline --password-file /root/password.txt get simple
}
