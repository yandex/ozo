FROM ubuntu:18.04

RUN apt-get update && \
    apt-get install -y \
        python3 \
        python3-pip \
        postgresql-client \
        && \
    rm -rf /var/lib/apt/lists/*

RUN pip3 install asyncpg

VOLUME /code

WORKDIR /code

ENV CCACHE_DIR=/ccache
