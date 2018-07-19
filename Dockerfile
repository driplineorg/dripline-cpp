FROM debian:9

# Most dependencies

RUN apt-get update && \
    apt-get clean && \
    apt-get --fix-missing  -y install \
        build-essential \
        cmake \
        gdb \
        libboost-all-dev \
        librabbitmq-dev \
        wget && \
    rm -rf /var/lib/apt/lists/*

# note that the build dir is *not* in source, this is so that the source can me mounted onto the container without covering the build target

COPY SimpleAmqpClient /usr/local/src/SimpleAmqpClient
COPY documentation /usr/local/src/documentation
COPY scarab /usr/local/src/scarab
COPY source /usr/local/src/source
COPY CMakeLists.txt /usr/local/src/CMakeLists.txt

RUN mkdir -p /usr/local/src/build && \
    cd /usr/local/src/build && \
    cmake .. && \
    # unclear why I have to run cmake twice
    cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr/local . && \
    make install

#RUN cp /usr/local/src/examples/str_1ch_fpa.yaml /etc/psyllid_config.yaml
