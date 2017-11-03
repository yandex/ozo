#!/bin/bash -ex

export CC='ccache clang'
export CXX='ccache clang++'

export ASAN_OPTIONS='halt_on_error=0:strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1'

ASAN_CXX_FLAGS='-fno-omit-frame-pointer -fsanitize=address -fsanitize-address-use-after-scope -fsanitize-recover=address'

BUILD_DIR=${BUILD_PREFIX}build_clang_asan
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}
cmake \
    -DCMAKE_CXX_FLAGS='-std=c++17 -Wall -Wextra -pedantic -Werror' \
    -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-g -O1 ${ASAN_CXX_FLAGS}" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DAPQ_BUILD_TESTS=ON \
    ..
make -j$(nproc)
ctest -V
