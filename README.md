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
