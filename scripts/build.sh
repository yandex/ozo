#!/bin/bash -ex

TARGET=$2

build_clang() {
    COMPILER=clang
    export CC='ccache clang'
    export CXX='ccache clang++'
    case "$TARGET" in
        debug)
            CMAKE_BUILD_TYPE=Debug
            build
        ;;
        release)
            CMAKE_BUILD_TYPE=RelWithDebInfo
            build
        ;;
        asan)
            export ASAN_OPTIONS='halt_on_error=0:strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1'
            ASAN_CXX_FLAGS='-fno-omit-frame-pointer -fsanitize=address -fsanitize-address-use-after-scope -fsanitize-recover=address'
            CMAKE_CXX_FLAGS_RELWITHDEBINFO="-g -O1 ${ASAN_CXX_FLAGS}"
            CMAKE_BUILD_TYPE=RelWithDebInfo
            build
        ;;
        ubsan)
            CMAKE_BUILD_TYPE=Debug
            CMAKE_CXX_FLAGS='-fno-omit-frame-pointer -fsanitize=undefined'
            build
        ;;
        *) usage "bad clang target '$TARGET'";;
    esac
}

build_gcc() {
    COMPILER=gcc
    export CC='ccache gcc'
    export CXX='ccache g++'
    case "$TARGET" in
        debug)
            CMAKE_BUILD_TYPE=Debug
            build
        ;;
        release)
            CMAKE_BUILD_TYPE=RelWithDebInfo
            build
        ;;
        coverage)
            CMAKE_BUILD_TYPE=Debug
            OZO_COVERAGE=ON
            build
        ;;
        *) usage "bad gcc target '$TARGET'";;
    esac
}

usage() {
    local NAME=`basename $0`
    echo $NAME: ERROR: $* 1>&2
    echo 'usage:
        '$NAME' all
            build all targets for all compilers
        '$NAME' <compiler> <target>
        Build with specified compiler or target
        compiler : gcc | clang
        target   :
            - for gcc   : debug | release | coverage
            - for clang : debug | release | asan | ubsan
        '$NAME' docker [all | <compiler> <target>]
        Build inside docker' 1>&2
    exit 1
}

build_all() {
    $0 gcc debug
    $0 gcc release
    $0 gcc coverage
    $0 clang debug
    $0 clang release
    $0 clang asan
    $0 clang ubsan
}

build() {
    BUILD_DIR=${BUILD_PREFIX}build/${COMPILER}_${TARGET}
    mkdir -p ${BUILD_DIR}
    cd ${BUILD_DIR}
    cmake \
        -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS"\
        -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="$CMAKE_CXX_FLAGS_RELWITHDEBINFO" \
        -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE \
        -DOZO_BUILD_TESTS=ON \
        -DOZO_COVERAGE=$OZO_COVERAGE\
        -DOZO_BUILD_PG_TESTS=$OZO_BUILD_PG_TESTS \
        -DOZO_PG_TEST_CONNINFO="$OZO_PG_TEST_CONNINFO" \
        ../..
    make -j$(nproc)
    if [[ $OZO_BUILD_PG_TESTS = "ON" ]]
    then
        while ! pg_isready -h localhost -p 5432; do
            echo "waiting until local postgres is accepting connections..."
            sleep 1
        done
    fi
    ctest -V
    if [[ ${OZO_COVERAGE} == "ON" ]]; then
        make ozo_coverage
    fi
}

launch_in_docker() {
    docker run -ti --net=host --rm --user "$(id -u):$(id -g)" --privileged -v ${HOME}/.ccache:/ccache -v ${PWD}:/code ozo_build env BUILD_PREFIX=docker_ env OZO_BUILD_PG_TESTS=${OZO_BUILD_PG_TESTS} env OZO_PG_TEST_CONNINFO="${OZO_PG_TEST_CONNINFO}" $0 $*
}

launch_with_pg() {
    PG_CONTAINER_ID=$(docker run -d \
    --name ozo-test-postgres \
    -e POSTGRES_DB=ozo_test_db \
    -e POSTGRES_USER=ozo_test_user \
    -e POSTGRES_PASSWORD=123 \
    -p 5432:5432/tcp \
    postgres:alpine)

    export OZO_BUILD_PG_TESTS=ON
    export OZO_PG_TEST_CONNINFO='host=0.0.0.0 port=5432 dbname=ozo_test_db user=ozo_test_user password=123'

    $* && docker stop $PG_CONTAINER_ID && docker rm $PG_CONTAINER_ID \
       || (docker stop $PG_CONTAINER_ID && docker rm $PG_CONTAINER_ID)
}

case "$1" in
    gcc|clang|all)
        build_${1}
        exit 0
    ;;
    docker)
        launch_in_docker $2 $3 $4 $5
        exit 0
    ;;
    pg)
        launch_with_pg $0 $2 $3 $4 $5
        exit 0
    ;;
    help|-h|--help)
        usage
    ;;
    *)
        usage "bad first parameter '$1'"
    ;;
esac
