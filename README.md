# ozo

[![Build Status](https://travis-ci.org/YandexMail/ozo.svg?branch=master)](https://travis-ci.org/YandexMail/ozo)
[![codecov](https://codecov.io/gh/YandexMail/ozo/branch/master/graph/badge.svg)](https://codecov.io/gh/YandexMail/ozo)

## What's this

OZO is a C++17 library for asyncronous communication with PostgreSQL DBMS.
The library leverages the power of template metaprogramming, providing convenient mapping from C++ types to SQL along with rich query building possibilities. OZO supports different concurrency paradigms (callbacks, futures, coroutines), using Boost.Asio under the hood. Low-level communication with PostgreSQL server is done via libpq. All concepts in the library are designed to be easily extendable (even replaceable) by the user to simplify adaptation to specific project requirements.

### API

Since the project is on early state of developmet it lacks of documentation. We understand the importance of good docs and are working hard on this problem. But until the work isn't done there are [unit tests](tests/request_integration.cpp) which can help you understand the main use-cases of OZO on this early state of the project. We also have a C++Now'18 talk about OZO. The persentation is [here](https://github.com/boostcon/cppnow_presentations_2018/blob/master/05-09-2018_wednesday/design_and_implementation_of_dbms_asynchronous_client_library__roman_siromakha__cppnow_05092018.pdf). Video will be provided later at [C++Now 2018 playlist](https://www.youtube.com/watch?v=QFIOE1jKv30&list=PL_AKIMJc4roVSbTTfHReQTl1dc9ms0lWH).

## Dependencies

These things are needed:

* **CMake** is used as build system
* **GCC** or **Clang** C++ compiler with C++17 support (tested with GCC 7.0, Clang 5.0 and Apple LLVM version 9.0.0)
* **Boost** >= 1.66
* **libpq** >= 9.3

If you want to run integration tests and/or build inside Docker container:
* **Docker** >= 1.13.0
* **Docker Compose** >= 1.10.0

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
* **asan** - for [AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html) launch
* **ubsan** - for [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html) launch
* **tsan** - for [ThreadSanitizer](https://clang.llvm.org/docs/ThreadSanitizer.html) launch

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

To build code and run tests inside docker container:

```bash
scripts/build.sh docker <compiler> <target>
```

### Test against a local postgres

You can use `scripts/build.sh` but add `pg` first:

```bash
scripts/build.sh pg <compiler> <target>
```

or if you want build code in docker:

```bash
scripts/build.sh pg docker <compiler> <target>
```

This will attempt to launch postgres:alpine from your Docker registry.
Or you can point ozo tests to a postgres of your choosing by setting these environment variables prior to building:

```bash
export OZO_BUILD_PG_TESTS=ON
export OZO_PG_TEST_CONNINFO='your conninfo (connection string)'

scripts/build.sh gcc debug
```
