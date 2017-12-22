FROM ubuntu:17.10

RUN apt-get update && apt-get install -y ccache clang-5.0 libpq-dev llvm-5.0 cmake g++-7 git wget && \
    rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-5.0 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-5.0 100 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 100 && \
    update-alternatives --install /usr/bin/llvm-symbolizer llvm-symbolizer /usr/bin/llvm-symbolizer-5.0 100

RUN wget -qO boost_1_64_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.64.0/boost_1_64_0.tar.gz/download && \
    tar xzf boost_1_64_0.tar.gz && \
    cd boost_1_64_0 && \
    ./bootstrap.sh --with-libraries=system,thread,chrono,date_time,context,coroutine && \
    ./b2 \
        -j $(nproc) \
        --reconfigure \
        link=static \
        threading=multi \
        variant=release \
        cxxflags='-std=c++17 -DBOOST_COROUTINES_NO_DEPRECATION_WARNING' \
        debug-symbols=on \
        warnings=off \
    install

VOLUME /ccache
VOLUME /code

WORKDIR /code

ENV CCACHE_DIR=/ccache
