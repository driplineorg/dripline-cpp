ARG img_repo=python
ARG img_tag=3.14.2-slim-trixie

# This FROM line includes a label so that the dependencies can be built by themselves by using the `--target` argument of `docker build`
FROM ${img_repo}:${img_tag} AS base

ARG build_type=Release
ARG build_examples=FALSE
ARG enable_testing=FALSE
ARG narg=2

# use pybind11_checkout to specify a tag or branch name to checkout
ARG pybind11_checkout=v3.0.1
ARG pybind11_repo=https://github.com/pybind/pybind11.git
ARG pybind11_name=pybind11

# Most dependencies
# The apt-provided cmake is new enough: bde requires 3.24 and rmqcpp requires 3.25, and Debian
# bookworm and trixie provide 3.25.1 and 3.31.6 respectively.
RUN apt-get update && \
    apt-get clean && \
    apt-get --fix-missing  -y install --no-install-recommends \
        build-essential \
        cmake \
#        gdb \
#        pybind11-dev \
        git \
        libyaml-cpp-dev \
        rapidjson-dev \
        libboost-dev \
        libboost-filesystem-dev \
        libboost-system-dev \
        libboost-chrono-dev \
        libssl-dev \
        zlib1g-dev \
        libzstd-dev \
        ninja-build \
        pkg-config && \
    rm -rf /var/lib/apt/lists/* && \
    # Pybind11 install
    cd /usr/local && \
    git clone ${pybind11_repo} ${pybind11_name} && \
    cd ${pybind11_name} && \
    git checkout ${pybind11_checkout} && \
    mkdir build && \
    cd build && \
    cmake -DPYBIND11_TEST=FALSE .. && \
    make -j${narg} install && \
    cd / && \
    rm -rf /usr/local/${pybind11_name}

# rmqcpp and the Bloomberg BDE libraries it is built on have no system packages, so they are
# built from source.  install_dependencies.sh clones and builds in a temporary directory, which
# it removes when it is done, so this stays a single self-contained layer.
#
# The pinned versions live in install_dependencies.sh and nowhere else.  These args default to
# empty and are passed on only when set, so that the script's own defaults apply otherwise.
ARG bde_version=
ARG rmqcpp_repo=
ARG rmqcpp_ref=

COPY install_dependencies.sh /usr/local/bin/install_dependencies.sh
RUN chmod +x /usr/local/bin/install_dependencies.sh && \
    /usr/local/bin/install_dependencies.sh \
        --prefix=/usr/local \
        --jobs=${narg} \
        ${bde_version:+--bde-version=${bde_version}} \
        ${rmqcpp_repo:+--rmqcpp-repo=${rmqcpp_repo}} \
        ${rmqcpp_ref:+--rmqcpp-ref=${rmqcpp_ref}} && \
    # Drop debug info from the installed dependency libraries.  The static archives get
    # --strip-debug rather than --strip-unneeded, which would discard symbols that consumers
    # of the archives still need to link against.
    find /usr/local/lib -name '*.a' -type f -exec strip --strip-debug {} + && \
    find /usr/local/lib -name '*.so*' -type f -exec strip --strip-unneeded {} +

FROM base AS devel

RUN apt-get update && \
    apt-get clean && \
    apt-get --fix-missing  -y install \
        nano \
        gdb \
        valgrind \
        cmake-curses-gui

FROM base

# note that the build dir is *not* in source, this is so that the source can me mounted onto the container without covering the build target

COPY .git /usr/local/src/.git
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
