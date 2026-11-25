#! /bin/bash

# Runs the test suite using docker compose
#
# Usage:
#  do-testing.sh [image tag]

# source: https://blog.harrison.dev/2016/06/19/integration-testing-with-docker-compose.html

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

cleanup () {
  docker compose -p integration kill
  docker compose -p integration rm -f
}
trap 'cleanup ; printf "${RED}Tests Failed For Unexpected Reasons${NC}\n"' HUP INT QUIT PIPE TERM

IMG_TAG=$1 docker compose -p integration -f docker-compose.yaml -f docker-compose-broker-restart-test.yaml build && IMG_TAG=$1 docker compose -p integration -f docker-compose.yaml -f docker-compose-broker-restart-test.yaml up -d rabbit-broker simple-service

# Wait for broker health
#echo "Waiting for broker to become healthy..."
#until docker exec integration-rabbit-broker-1 \
#    curl -sf -u dripline:dripline http://localhost:15672/api/overview > /dev/null 2>&1; do
#    echo "  broker not yet healthy, waiting..."
#    sleep 2
#done

# Pause to allow the service to connect
sleep 5

# Restart broker
#   Broker will restart and simple-service will continually try to reconnect
#   Test for service connection (dl-agent cmd -s ping simple) will wait until broker is back
docker compose -p integration restart rabbit-broker

# Now run the test container
IMG_TAG=$1 docker compose -p integration -f docker-compose.yaml -f docker-compose-broker-restart-test.yaml up --exit-code-from test test

if [ $? -ne 0 ] ; then
  printf "${RED}Docker Compose Failed${NC}\n"
  exit -1
fi

TEST_EXIT_CODE=`docker wait integration-test-1`
docker logs integration-test-1
if [ -z ${TEST_EXIT_CODE+x} ] || [ "$TEST_EXIT_CODE" -ne 0 ] ; then
  docker logs integration-test-1
  docker logs integration-simple-service-1
  docker logs integration-rabbit-broker-1
  printf "${RED}Tests Failed${NC} - Exit Code: $TEST_EXIT_CODE\n"
else
  printf "${GREEN}Tests Passed${NC}\n"
fi

cleanup

exit $TEST_EXIT_CODE
