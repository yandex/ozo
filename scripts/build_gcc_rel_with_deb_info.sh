#!/bin/bash -ex

export CC='ccache gcc'
export CXX='ccache g++'

BUILD_DIR=${BUILD_PREFIX}build_gcc_rel_with_deb_info
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}
cmake \
    -DCMAKE_CXX_FLAGS='-std=c++17 -Wall -Wextra -pedantic -Werror' \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DAPQ_BUILD_TESTS=ON \
    ..
make -j$(nproc)
ctest -V
