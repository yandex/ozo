# ozo

[![Build Status](https://travis-ci.org/YandexMail/ozo.svg?branch=master)](https://travis-ci.org/YandexMail/ozo)
[![Coverage Status](https://coveralls.io/repos/github/YandexMail/ozo/badge.svg?branch=master)](https://coveralls.io/github/YandexMail/ozo?branch=master)

## Dependencies

These things are needed:

* **CMake** is used as build system
* **GCC** or **Clang** C++ compiler with C++17 support (tested with GCC 7.0, Clang 5.0 and Apple LLVM version 9.0.0)
* **Boost** >= 1.66
* **libpq** >= 9.3

## Build

The library is header-only, but if you want to build and run unit-tests you can do it as listed below.

### Build and run tests on custom environment

First of all you need to satsfy requirements listed above. You can run tests using these commands.

```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
ctest -V
```

Or use [build.sh](scripts/build.sh) which accepts folowing commands:

```bash
scripts/build.sh help
```

prints help.

```bash
scripts/build.sh <compiler> <target>
```

build and run tests with specified **compiler** and **target**, the **compiler** parameter can be:

* **gcc** - for build with gcc,
* **clang** - for build with clang.

The **target** parameter depends on **compiler**.
For **gcc**:

* **debug** - for debug build and tests
* **release** - for release build and tests
* **coverage** - for code coverage calculation

For **clang**:

* **debug** - for debug build and tests
* **release** - for release build and tests
* **asan** - for address sanitaizer launch
* **ubsan** - for UB sanitaizer launch

```bash
scripts/build.sh all
```

build all possible configuration.

### Build and run tests on MacOS 10.X

For MacOS the best way to satisfy minimum requirements is [brew](https://brew.sh/)

```bash
brew install cmake boost libpq postresql
```

### Build and run tests within Docker

First of all build docker image:

```bash
scripts/build_docker_image_build.sh
```

To build code and run tests inside docker container:

```bash
scripts/build_inside_docker.sh <script to build>
```

### Test against a local postgres

You can use scripts/with_live_pg_tests.sh from project root like this:

```bash
scripts/with_live_pg_tests.sh scripts/build.sh gcc debug
```

This will attempt to launch postgres/alpine from your Docker registry.
Or you can point ozo tests to a postgres of your choosing by setting these environment variables prior to building:

```bash
export OZO_BUILD_PG_TESTS=ON
export OZO_PG_TEST_CONNINFO='your conninfo (connection string)'

scripts/build_gcc_debug.sh
```