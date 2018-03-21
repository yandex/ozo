# ozo

[![Build Status](https://travis-ci.org/YandexMail/ozo.svg?branch=master)](https://travis-ci.org/YandexMail/ozo)
[![Coverage Status](https://coveralls.io/repos/github/YandexMail/ozo/badge.svg?branch=HEAD)](https://coveralls.io/github/YandexMail/ozo?branch=HEAD)

## Build

### With custom environment

Requirements:
* CMake
* C compiler
* C++ compiler with C++17 support
* Boost

```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

Or just use one of the scripts:
* [scripts/build_gcc_debug.sh](scripts/build_gcc_debug.sh)
* [scripts/build_gcc_rel_with_deb_info.sh](scripts/build_gcc_rel_with_deb_info.sh)
* [scripts/build_gcc_coverage.sh](scripts/build_gcc_coverage.sh)
* [scripts/build_clang_debug.sh](scripts/build_clang_debug.sh)
* [scripts/build_clang_rel_with_deb_info.sh](scripts/build_clang_rel_with_deb_info.sh)
* [scripts/build_clang_asan.sh](scripts/build_clang_asan.sh)
* [scripts/build_clang_ubsan.sh](scripts/build_clang_ubsan.sh)

### With Docker

Build docker image:
```bash
scripts/build_docker_image.sh
```

Build code inside docker container:
```bash
scripts/build_inside_docker.sh
```

or with custom build script:
```bash
scripts/build_inside_docker.sh scripts/build_gcc_debug.sh
```

### Test against a local postgres

You can use scripts/with_live_pg_tests.sh from project root like this:
```bash
scripts/with_live_pg_tests.sh scripts/build_gcc_debug.sh
```
This will attempt to launch postgres/alpine from your Docker registry.
Or you can point ozo tests to a postgres of your choosing by setting these environment variables prior to building:
```bash
export OZO_BUILD_PG_TESTS=ON
export OZO_PG_TEST_CONNINFO='your conninfo (connection string)'

scripts/build_gcc_debug.sh
```