ARG img_repo=python
ARG img_tag=3.14.2-slim-trixie

# This FROM line includes a label so that the dependencies can be built by themselves by using the `--target` argument of `docker build`
FROM ${img_repo}:${img_tag} AS base

ARG build_type=Release
ARG build_examples=FALSE
ARG enable_testing=FALSE
ARG narg=2

ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_ROOT=/usr/local/vcpkg

# Most dependencies
RUN apt-get update && \
    apt-get clean && \
    apt-get --fix-missing  -y install \
        build-essential \
        cmake \
#        gdb \
        git \
        libyaml-cpp-dev \
        rapidjson-dev \
        libssl-dev \
        zlib1g-dev \
        libzstd-dev \
        ninja-build \
        pkg-config \
        curl \
        tar \
        unzip \
        zip && \
#        pybind11-dev \
#        wget && \
    rm -rf /var/lib/apt/lists/*

# use pybind11_checkout to specify a tag or branch name to checkout
ARG pybind11_checkout=v3.0.1
ARG pybind11_repo=https://github.com/pybind/pybind11.git
ARG pybind11_name=pybind11
RUN cd /usr/local && \
    git clone ${pybind11_repo} ${pybind11_name} && \
    cd ${pybind11_name} && \
    git checkout ${pybind11_checkout} && \
    mkdir build && \
    cd build && \
    cmake -DPYBIND11_TEST=FALSE .. && \
    make -j${narg} install && \
    cd / && \
    rm -rf /usr/local/${pybind11_name}

ARG TARGETARCH
ARG rmqcpp_checkout=
RUN cd /usr/local && \
    git clone https://github.com/Microsoft/vcpkg.git && \
    /usr/local/vcpkg/bootstrap-vcpkg.sh && \
    git clone https://github.com/bloomberg/rmqcpp.git && \
    cd /usr/local/rmqcpp && \
    git checkout ${rmqcpp_checkout} && \
    case "${TARGETARCH}" in \
        amd64) TRIPLET="x64-linux-release" ;; \
        arm64) TRIPLET="arm64-linux-release" ;; \
        *) echo "Unsupported architecture: ${TARGETARCH}"; exit 1 ;; \
    esac && \
    ${VCPKG_ROOT}/vcpkg install --triplet ${TRIPLET} && \
    mkdir build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
        -DVCPKG_TARGET_TRIPLET=${TRIPLET} \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DBUILD_TESTING=OFF \
        .. && \
    make -j${narg} install && \
    cd / && \
    ${VCPKG_ROOT}/vcpkg install --triplet ${TRIPLET} boost-filesystem boost-system boost-chrono boost-variant && \
    cp -a /usr/local/rmqcpp/vcpkg_installed/${TRIPLET}/include/. /usr/local/include/ && \
    cp -a /usr/local/rmqcpp/vcpkg_installed/${TRIPLET}/lib/. /usr/local/lib/ && \
    cp -a /usr/local/rmqcpp/vcpkg_installed/${TRIPLET}/share/. /usr/local/share/ && \
    cp -a ${VCPKG_ROOT}/installed/${TRIPLET}/include/. /usr/local/include/ && \
    cp -a ${VCPKG_ROOT}/installed/${TRIPLET}/lib/. /usr/local/lib/ && \
    cp -a ${VCPKG_ROOT}/installed/${TRIPLET}/share/. /usr/local/share/ && \
    rm -rf /usr/local/share/zstd && \
    # Create a shim file to adapt the vcpkg install of "pcre2" to the expected standard "libpcre2-8"
    mkdir -p /usr/local/share/libpcre2-8 && \
    printf '%s\n' \
        'include("/usr/local/share/pcre2/pcre2-config.cmake")' \
        'if(NOT TARGET libpcre2-8::pcre2-8 AND TARGET pcre2::pcre2-8-static)' \
        '  add_library(libpcre2-8::pcre2-8 ALIAS pcre2::pcre2-8-static)' \
        'endif()' \
        'set(libpcre2-8_FOUND TRUE)' \
        > /usr/local/share/libpcre2-8/libpcre2-8Config.cmake && \
    # rmqcpp's rmqcppConfig.cmake is missing the use of find_dependency(zstd), so we add it
    RMQCPP_CONFIG=$(find /usr/local -name rmqcppConfig.cmake 2>/dev/null | head -1) && \
    test -n "${RMQCPP_CONFIG}" && \
    if ! grep -q 'find_dependency(zstd' "${RMQCPP_CONFIG}"; then \
        sed -i '/include.*rmqcppTargets/i find_dependency(zstd CONFIG)' "${RMQCPP_CONFIG}"; \
    fi
    #rm -rf /usr/local/rmqcpp /usr/local/vcpkg /root/.cache/vcpkg

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
