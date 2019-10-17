#!/bin/bash -ex

docker-compose build asyncpg aiopg ozo_build_with_pg_tests

scripts/build.sh pg docker clang release

function run_benchmark {
    docker-compose run \
        --rm \
        --user "$(id -u):$(id -g)" \
        ${1:?} \
        bash \
        -exc "/code/scripts/wait_postgres.sh; ${2:?} ${3:?}"
}

run_benchmark asyncpg benchmarks/asyncpg_benchmark.py '"postgresql://${POSTGRES_USER}:${POSTGRES_PASSWORD}@${POSTGRES_HOST}/${POSTGRES_DB}"'
run_benchmark aiopg benchmarks/aiopg_benchmark.py '"host=${POSTGRES_HOST} user=${POSTGRES_USER} dbname=${POSTGRES_DB} password=${POSTGRES_PASSWORD}"'
run_benchmark ozo_build_with_pg_tests '${BASE_BUILD_DIR}/clang_release/benchmarks/ozo_benchmark' \
    '"host=${POSTGRES_HOST} user=${POSTGRES_USER} dbname=${POSTGRES_DB} password=${POSTGRES_PASSWORD}"'
run_benchmark ozo_build_with_pg_tests '${BASE_BUILD_DIR}/clang_release/benchmarks/ozo_benchmark_performance' \
    '"host=${POSTGRES_HOST} user=${POSTGRES_USER} dbname=${POSTGRES_DB} password=${POSTGRES_PASSWORD}"'

docker-compose stop ozo_postgres
docker-compose rm -f ozo_postgres
