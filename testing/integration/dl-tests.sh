#! /usr/bin/env bats

@test "ping simple" {
    dl-agent -vv cmd simple -s ping
}

@test "get simple" {
    dl-agent -vv  get simple
}

@test "set simple" {
    dl-agent -vv set simple 500
}

@test "multi-chunk set" {
    LARGE_VALUE=$(python3 -c "print('x' * 12000)")
    dl-agent -vv --max-payload 500 set simple "${LARGE_VALUE}"
}

@test "user/pass on CL" {
    dl-agent -vv -u dripline --password dripline get simple
}

@test "user on CL/pass in file" {
    dl-agent -vv -u dripline --password-file /dl_pw.txt get simple
}

@test "auth file" {
    dl-agent -vv --auth-file /auths.json get simple
}

@test "oscillator hub" {
    dl-agent -vv get osc_svc_hub -s in-phase
}

@test "oscillator endpoints" {
    dl-agent -vv get in_phase
}
