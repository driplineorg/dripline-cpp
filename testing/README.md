# README for dripline-cpp/testing

## Catch2

Dripline-cpp uses the Catch2 testing framework for unit tests.  See these links for more information:

* [Main GitHub page](https://github.com/catchorg/Catch2)
* [Tutorial](https://github.com/catchorg/Catch2/blob/master/docs/tutorial.md)

## Building tests

In CMake, enable the option `Dripline_ENABLE_TESTING`, and build.

The testing executable, `run_tests`, will be installed in `install/prefix/testing`.

## Running tests

To see the available options for running tests, you can do:

```
> testing/run_tests -h
```

To simply run all of the tests, you can do:

```
> testing/run_tests
```

Or if you want to run a specific test, you can do:

```
> testing/run_tests [name]
```

For further documentation on using Catch2, see the tutorial at