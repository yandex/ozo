#!/bin/bash -ex

scripts/build_gcc_debug.sh
scripts/build_gcc_rel_with_deb_info.sh
scripts/build_clang_debug.sh
scripts/build_clang_rel_with_deb_info.sh
scripts/build_clang_asan.sh
scripts/build_clang_ubsan.sh
