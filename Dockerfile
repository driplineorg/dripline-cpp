ARG img_repo=python
ARG img_tag=3.12.1-slim-bookworm

# This FROM line includes a label so that the dependencies can be built by themselves by using the `--target` argument of `docker build`
FROM ${img_repo}:${img_tag} AS base

ARG build_type=Release
ARG build_examples=FALSE
ARG enable_testing=FALSE
ARG narg=2

# Most dependencies

RUN apt-get update && \
    apt-get clean && \
    apt-get --fix-missing  -y install \
        build-essential \
        cmake \
#        gdb \
        git \
        libboost-chrono-dev \
        libboost-filesystem-dev \
        libboost-system-dev \
        librabbitmq-dev \
        libyaml-cpp-dev \
        rapidjson-dev && \
#        pybind11-dev \
#        wget && \
    rm -rf /var/lib/apt/lists/*

# use pybind11_checkout to specify a tag or branch name to checkout
ARG pybind11_checkout=v2.13.5
RUN cd /usr/local && \
    git clone https://github.com/pybind/pybind11.git && \
    cd pybind11 && \
    git checkout ${pybind11_checkout} && \
    mkdir build && \
    cd build && \
    cmake -DPYBIND11_TEST=FALSE .. && \
    make -j${narg} install && \
    cd / && \
    rm -rf /usr/local/pybind11

# use pybind11_checkout to specify a tag or branch name to checkout
ARG plog_checkout=1.1.10
RUN cd /usr/local && \
    git clone https://github.com/SergiusTheBest/plog.git && \
    cd plog && \
    git checkout ${plog_checkout} && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j${narg} install && \
    cd / && \
    rm -rf /usr/local/plog


FROM base

# note that the build dir is *not* in source, this is so that the source can me mounted onto the container without covering the build target

COPY .git /usr/local/src/.git
COPY external /usr/local/src/external
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
    cmake -DCMAKE_BUILD_TYPE=${build_type} \
        -DCMAKE_INSTALL_PREFIX:PATH=/usr/local \ 
        -DDripline_BUILD_EXAMPLES:BOOL=${build_examples} \
        -DDripline_ENABLE_TESTING:BOOL=${enable_testing} \
        -DDripline_BUILD_PYTHON:BOOL=TRUE \
        -DPBUILDER_PY_INSTALL_IN_SITELIB=TRUE \
        ../src && \
    make -j${narg} install

# TODO: rm -rf /usr/local/src
