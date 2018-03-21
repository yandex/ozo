#!/bin/bash -ex

export CC='ccache gcc'
export CXX='ccache g++'

BUILD_DIR=${BUILD_PREFIX}build_gcc_coverage
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}
cmake \
    -DCMAKE_CXX_FLAGS='-std=c++17 -Wall -Wextra -pedantic -Werror' \
    -DCMAKE_BUILD_TYPE=Debug \
    -DOZO_BUILD_TESTS=ON \
    -DOZO_COVERAGE=ON \
    -DOZO_BUILD_PG_TESTS=$OZO_BUILD_PG_TESTS \
    -DOZO_PG_TEST_CONNINFO="$OZO_PG_TEST_CONNINFO" \
    -DPostgreSQL_TYPE_INCLUDE_DIR=/usr/include/postgresql/ \
    ..
make -j$(nproc)
make ozo_coverage
