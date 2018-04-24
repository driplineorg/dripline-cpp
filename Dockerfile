FROM debian:8

# Most dependencies

RUN apt-get update && \
    apt-get clean && \
    apt-get --fix-missing  -y install \
        build-essential \
        #libfftw3-3 \
        #libfftw3-dev \
        gdb \
        libboost-all-dev \
        #libhdf5-dev \
        librabbitmq-dev \
        wget && \
    rm -rf /var/lib/apt/lists/*

# CMake
ARG CMAKEVER=3.6
ARG CMAKEINSTALLER=cmake-3.6.2-Linux-x86_64.sh
RUN wget https://cmake.org/files/v$CMAKEVER/$CMAKEINSTALLER && \
    chmod u+x $CMAKEINSTALLER && \
    ./$CMAKEINSTALLER --skip-license --prefix=/usr/local && \
    rm $CMAKEINSTALLER

# note that the build dir is *not* in source, this is so that the source can me mounted onto the container without covering the build target

COPY SimpleAmqpClient /usr/local/src/SimpleAmqpClient
COPY documentation /usr/local/src/documentation
COPY scarab /usr/local/src/scarab
COPY source /usr/local/src/source
COPY CMakeLists.txt /usr/local/src/CMakeLists.txt

RUN mkdir -p /usr/local/src/build && \
    cd /usr/local/src/build && \
    cmake .. && \
    # unclear while I have to run cmake twice
    cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr/local . && \
    make install

#RUN cp /usr/local/src/examples/str_1ch_fpa.yaml /etc/psyllid_config.yaml
