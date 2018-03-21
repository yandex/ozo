#!/bin/bash -ex

PG_CONTAINER_ID=$(docker run -d \
    --name ozo-test-postgres \
    -e POSTGRES_DB=ozo_test_db \
    -e POSTGRES_USER=ozo_test_user \
    -e POSTGRES_PASSWORD=123 \
    -p 5432:5432/tcp \
    postgres:alpine)

while ! pg_isready -h localhost -p 5432; do
    echo "waiting until local postgres is accepting connections..."
    sleep 1
done

export OZO_BUILD_PG_TESTS=ON
export OZO_PG_TEST_CONNINFO='host=localhost port=5432 dbname=ozo_test_db user=ozo_test_user password=123'

./"$1" && docker stop $PG_CONTAINER_ID && docker rm $PG_CONTAINER_ID \
       || (docker stop $PG_CONTAINER_ID && docker rm $PG_CONTAINER_ID)