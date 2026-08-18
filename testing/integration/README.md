# Integration Tests

## Docker Container

This directory contains a Dockerfile that adds a few utilities used in the integration testing to the dripline-cpp image.
If you build the image, we recommend you tag the image with something like `ghcr.io/driplineorg/dripline-cpp:[version tag]-test`:

    > docker build --build-arg img_tag=[version tag]-dev -t ghcr.io/driplineorg/dripline-cpp:[version tag]-test .

## The Tests

There are two independent test suites, each with its own convenience script and Docker Compose extension file.

### DL Tests (`run-dl-tests.sh`)

This suite exercises broker setup and Dripline service functionality.  It uses `docker-compose-test.yaml` as the Docker Compose extension.

#### Broker Setup

The first tests run demonstrate that the RabbitMQ broker has been configured correctly once the services are connected.  We check that the expected exchanges and queues are in place by sending HTTP requests to the broker and validating the responses.  The `newman` CLI application is used to run a Postman collection.

The command to run the collection is:

    > newman run /path/to/rabbitmq_for_dl_collection.json

The collection file is setup to run in the Docker compose environment, where the broker is addressed at `rabbit-broker`.  If you want to run it on a host machine on which the Docker setup is running and port 15672 has been opened, you can change instances of `rabbit-broker` in the JSON file to `localhost`, and use the same command.

#### Dripline Services

We test use of Dripline services by sending requests to those services and validating the results.  The tests are specified in `dl-tests.sh`.  They're run and checked using the [bats framework](https://bats-core.readthedocs.io/en/stable/index.html). Currently the tests include:

* `ping simple` — sends a ping command to the simple service
* `get simple` — sends a get request to the simple service
* `set simple 500` — sends a set request to the simple service
* `multi-chunk set` — sends a large set request that is split into multiple AMQP message chunks
* `user/pass on CL` — verifies authentication with username and password supplied on the command line
* `user on CL/pass in file` — verifies authentication with username on the command line and password read from a file
* `auth file` — verifies authentication using a JSON auth file
* `oscillator hub` — sends a get request to the oscillator hub service
* `oscillator endpoints` — sends a get request to an oscillator endpoint service

You can run these tests directly by executing the script:

    > /path/to/dl-tests.sh

#### Running the DL Tests

You can run the tests directly with `docker compose`:

    > IMG_TAG=[image tag] docker compose -f docker-compose.yaml -f docker-compose-test.yaml up

Or you can use the convenience script, `run-dl-tests.sh`:

    > ./run-dl-tests.sh [image tag]

Note that the terminal output is suppressed using the `run-dl-tests.sh` script.  Output from the test container will be printed.  Output from the service(s) in use will be printed on error.

---

### Broker-Restart Test (`run-broker-restart-test.sh`)

This suite verifies that Dripline services automatically reconnect to the broker after the broker is restarted.  It uses `docker-compose-broker-restart-test.yaml` as the Docker Compose extension.

The test sequence is:

1. Start the broker (`rabbit-broker`) and the simple service (`simple-service`).
2. Wait 5 seconds for the service to connect.
3. Restart the broker with `docker compose restart rabbit-broker`.
4. Start the test container, which waits for the broker to recover and then waits an additional 2 seconds for the service to reconnect.
5. Send a ping to the simple service to confirm it has successfully reconnected.

The test is specified in `broker-restart-test.sh` and run with the [bats framework](https://bats-core.readthedocs.io/en/stable/index.html).

#### Running the Broker-Restart Test

    > ./run-broker-restart-test.sh [image tag]

---

## Implementation Notes

* Synchronizing containers
  * Anything using the broker needs to wait until the broker is ready.  This is done using a (Docker) healthcheck.  The health status of the broker is determined by a curl request to the HTTP server that RabbitMQ comes with.  Once it has a healthy status, the DL services start.
  * Sending a request to a service requires that the service is running; so far we wait for 1 second after the broker is ready, and assume the service will be running.  This could be changed in the future to do something like wait-for-broker.sh except by sending pings to the relevant service.
* To include a test docker-compose extension, add a second `-f` argument, e.g.:
    > docker compose -f docker-compose.yaml -f docker-compose-test.yaml up
* Specify the docker image tag by setting an environment variable when you run, e.g.
    > IMG_TAG=v[version]-dev docker compose -f docker-compose.yaml up
  * There's a default tag (latest-dev), so if this isn't specified, everything should still run.
