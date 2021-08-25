#! /bin/bash

source /root/bash_test_tools

function setup
{
    # wait to make sure service connects
    sleep 2s
}


function test_get
{
    dl-agent -vv -b rabbit_broker --auth-file /root/authentication.json get simple

    assert_terminated_normally
    assert_exit_success
    assert_output_contains "Return Message: Congrats, you performed an OP_GET"
}

testrunner
