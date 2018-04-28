#!/bin/bash -ex

scripts/build.sh pg docker clang release

docker-compose up -d ozo_postgres

docker-compose run \
    --rm \
    --user "$(id -u):$(id -g)" \
    ozo_build_with_pg_tests \
    bash \
    -exc '/code/scripts/wait_postgres.sh; ${BASE_BUILD_DIR}/clang_release/examples/ozo_request "host=${POSTGRES_HOST} user=${POSTGRES_USER} dbname=${POSTGRES_DB} password=${POSTGRES_PASSWORD}"'

docker-compose run \
    --rm \
    --user "$(id -u):$(id -g)" \
    ozo_build_with_pg_tests \
    bash \
    -exc '/code/scripts/wait_postgres.sh; ${BASE_BUILD_DIR}/clang_release/examples/ozo_connection_pool "host=${POSTGRES_HOST} user=${POSTGRES_USER} dbname=${POSTGRES_DB} password=${POSTGRES_PASSWORD}"'

docker-compose stop ozo_postgres
docker-compose rm -f ozo_postgres
