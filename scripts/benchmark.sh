#!/bin/bash -ex

docker-compose build asyncpg aiopg ozo_build_with_pg_tests

scripts/build.sh pg docker clang release

echo 'asyncpg benchmark'

docker-compose run \
    --rm \
    --user "$(id -u):$(id -g)" \
    asyncpg \
    bash \
    -exc '/code/scripts/wait_postgres.sh; benchmarks/asyncpg_benchmark.py "postgresql://${POSTGRES_USER}:${POSTGRES_PASSWORD}@${POSTGRES_HOST}/${POSTGRES_DB}"'

echo 'aiopg benchmark'

docker-compose run \
    --rm \
    --user "$(id -u):$(id -g)" \
    aiopg \
    bash \
    -exc '/code/scripts/wait_postgres.sh; benchmarks/aiopg_benchmark.py "host=${POSTGRES_HOST} user=${POSTGRES_USER} dbname=${POSTGRES_DB} password=${POSTGRES_PASSWORD}"'

echo 'libpq benchmark'

docker-compose run \
    --rm \
    --user "$(id -u):$(id -g)" \
    ozo_benchmark \
    bash \
    -exc '/code/scripts/wait_postgres.sh; ${BASE_BUILD_DIR}/clang_release/benchmarks/libpq_benchmark "host=${POSTGRES_HOST} user=${POSTGRES_USER} dbname=${POSTGRES_DB} password=${POSTGRES_PASSWORD}"'

echo 'ozo benchmark'

docker-compose run \
    --rm \
    --user "$(id -u):$(id -g)" \
    ozo_benchmark \
    bash \
    -exc '/code/scripts/wait_postgres.sh; ${BASE_BUILD_DIR}/clang_release/benchmarks/ozo_benchmark "host=${POSTGRES_HOST} user=${POSTGRES_USER} dbname=${POSTGRES_DB} password=${POSTGRES_PASSWORD}"'

echo 'ozo performance benchmark'

docker-compose run \
   --rm \
   --user "$(id -u):$(id -g)" \
   ozo_build_with_pg_tests \
   bash \
   -exc '/code/scripts/wait_postgres.sh; ${BASE_BUILD_DIR}/clang_release/benchmarks/ozo_benchmark_performance "host=${POSTGRES_HOST} user=${POSTGRES_USER} dbname=${POSTGRES_DB} password=${POSTGRES_PASSWORD}"'

docker-compose stop ozo_postgres
docker-compose rm -f ozo_postgres
