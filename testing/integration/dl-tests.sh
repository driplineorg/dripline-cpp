#! /usr/bin/env bats

@test "ping simple" {
    dl-agent -vv -b rabbit-broker -u dripline --password-file /dl_pw.txt  cmd simple -s ping
}

@test "get simple" {
    dl-agent -vv -b rabbit-broker -u dripline --password-file /dl_pw.txt  get simple
}

@test "set simple" {
    dl-agent -vv -b rabbit-broker -u dripline --password-file /dl_pw.txt  set simple 500
}

@test "user/pass on CL" {
    dl-agent -vv -b rabbit-broker -u dripline --password dripline get simple
}

@test "user on CL/pass in file" {
    dl-agent -vvv -b rabbit-broker -u dripline --password-file /dl_pw.txt get simple
}

@test "user/pass as environment variables" {
    DRIPLINE_USER=dripline DRIPLINE_PASSWORD=dripline dl-agent -vvv -b rabbit-broker get simple
}

@test "auth file" {
    dl-agent -vv -b rabbit-broker --auth-file /auths.json get simple
}
