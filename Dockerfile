##FROM debian:9
FROM amd64/python:3.7

# Most dependencies

RUN apt-get update && \
    apt-get clean && \
    apt-get --fix-missing  -y install \
        build-essential \
        cmake \
        gdb \
        git \
        libboost-chrono-dev \
        libboost-filesystem-dev \
        libboost-system-dev \
        librabbitmq-dev \
        pybind11-dev \
        wget && \
    rm -rf /var/lib/apt/lists/*

RUN mkdir -p /usr/local/deps && \
    git clone https://github.com/jbeder/yaml-cpp && \
    cd yaml-cpp && \
    mkdir build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
        -DYAML_CPP_BUILD_CONTRIB:BOOL=FALSE \
        -DYAML_CPP_BUILD_TOOLS:BOOL=FALSE \
        -DYAML_CPP_BUILD_TESTS:BOOL=FALSE \
        -DYAML_BUILD_SHARED_LIBS=TRUE \
        .. && \
    make install 

RUN cd /usr/local/deps && \
    git clone https://github.com/Tencent/rapidjson && \
    cd rapidjson && \
    mkdir build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
        -DRAPIDJSON_BUILD_DOC=FALSE \
        -DRAPIDJSON_BUILD_EXAMPLES=FALSE \
        -DRAPIDJSON_BUILD_TESTS=FALSE \
        -DRAPIDJSON_BUILD_THIRDPARTY_GTEST=FALSE \
        .. && \
    make install

# note that the build dir is *not* in source, this is so that the source can me mounted onto the container without covering the build target

COPY .git /usr/local/src/.git
COPY external/SimpleAmqpClient /usr/local/src/external/SimpleAmqpClient
COPY documentation /usr/local/src/documentation
COPY scarab /usr/local/src/scarab
COPY library /usr/local/src/library
COPY executables /usr/local/src/executables
COPY testing /usr/local/src/testing
COPY examples /usr/local/src/examples
COPY CMakeLists.txt /usr/local/src/CMakeLists.txt
COPY dripline_shield.json /usr/local/src/dripline_shield.json
COPY DriplineConfig.cmake.in /usr/local/src/DriplineConfig.cmake.in

RUN mkdir -p /usr/local/build && \
    cd /usr/local/build && \
    cmake ../src && \
    # unclear why I have to run cmake twice
    cmake -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX:PATH=/usr/local \ 
        -DDripline_BUILD_EXAMPLES:BOOL=FALSE \
        -DDripline_BUILD_PYTHON=TRUE \
        ../src && \
    make install
