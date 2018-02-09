#!/bin/bash -ex

export CC='ccache clang'
export CXX='ccache clang++'

UBSAN_CXX_FLAGS='-fno-omit-frame-pointer -fsanitize=undefined'

BUILD_DIR=${BUILD_PREFIX}build_clang_ubsan
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}
cmake \
    -DCMAKE_CXX_FLAGS="-std=c++17 -Wall -Wextra -pedantic -Werror ${UBSAN_CXX_FLAGS}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DOZO_BUILD_TESTS=ON \
    -DOZO_BUILD_EXAMPLES=ON \
    -DPostgreSQL_TYPE_INCLUDE_DIR=/usr/include/postgresql/ \
    ..
make -j$(nproc)
ctest -V
