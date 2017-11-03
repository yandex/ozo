#!/bin/bash -ex

if ! [[ "${@}" ]]; then
    BUILD_SCRIPT='scripts/build_gcc_debug.sh'
else
    BUILD_SCRIPT="${@}"
fi

# https://github.com/google/sanitizers/issues/764
docker run -ti --rm --user "$(id -u):$(id -g)" --privileged -v ${HOME}/.ccache:/ccache -v ${PWD}:/code apq_build env BUILD_PREFIX=docker_ ${BUILD_SCRIPT}
