#! /bin/bash
#
# install_dependencies.sh
#
# Builds and installs the dripline-cpp dependencies that are not available as
# system packages: rmqcpp, and the Bloomberg BDE libraries that rmqcpp is built
# on.  Everything is built from pinned sources with plain CMake; vcpkg is not
# used or required.
#
# This script does *not* install system packages.  See --help for the list that
# must be present beforehand.
#
# Usage:
#     ./install_dependencies.sh --prefix=/usr/local --jobs=8
#
# Then configure dripline-cpp against the same prefix:
#     cmake -S . -B build -DCMAKE_PREFIX_PATH=<prefix>
#
# Noah Oblath, 2026
#

set -euo pipefail

###############################################################################
# Pinned dependency versions
#
# This is the only place the pinned versions live.  The Dockerfile and
# .github/workflows/publish.yaml both defer to these defaults; the Dockerfile
# build args exist only to override them.
#
# bde and bde-tools are released in lockstep and share a tag.
###############################################################################

BDE_REPO="${BDE_REPO:-https://github.com/bloomberg/bde.git}"
BDE_VERSION="${BDE_VERSION:-4.39.0.0}"

BDE_TOOLS_REPO="${BDE_TOOLS_REPO:-https://github.com/bloomberg/bde-tools.git}"
BDE_TOOLS_VERSION="${BDE_TOOLS_VERSION:-}"  # empty means "same tag as bde"

RMQCPP_REPO="${RMQCPP_REPO:-https://github.com/driplineorg/rmqcpp.git}"
RMQCPP_REF="${RMQCPP_REF:-6f00fab8d4e5cd90f1548fc90679dcff8f3a753f}"

###############################################################################
# Other defaults
###############################################################################

PREFIX="${PREFIX:-/usr/local}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
SRC_DIR="${SRC_DIR:-}"          # empty means "make a temporary directory"
KEEP_SOURCES="${KEEP_SOURCES:-FALSE}"
COMPRESSION="${COMPRESSION:-ON}"
GENERATOR="${GENERATOR:-}"      # empty means "Ninja if available"
OPENSSL_ROOT="${OPENSSL_ROOT:-}"
EXTRA_PREFIX_PATH="${EXTRA_PREFIX_PATH:-}"
JOBS="${JOBS:-}"
VERIFY_ONLY="${VERIFY_ONLY:-FALSE}"

# The minimum CMake that the dependencies themselves require: bde needs 3.24, rmqcpp needs 3.25
REQUIRED_CMAKE_VERSION=3.25

# Written into the install prefix so that the dripline-cpp build can be pointed at the same
# CMAKE_PREFIX_PATH and the same OpenSSL that rmqcpp was built against
DEPS_ENV_NAME=dripline-deps.env

###############################################################################
# Usage
###############################################################################

usage()
{
    cat <<EOF
Build and install the dripline-cpp dependencies that have no system packages:
rmqcpp and the Bloomberg BDE libraries.  vcpkg is not used.

Usage: $( basename "$0" ) [options]

Options:
  --prefix=DIR            Install prefix (default: ${PREFIX})
  --jobs=N                Parallel build jobs (default: number of CPUs)
  --build-type=TYPE       CMake build type (default: ${BUILD_TYPE})
  --generator=GEN         CMake generator (default: Ninja if available)
  --src-dir=DIR           Where to clone and build.  Defaults to a temporary
                          directory, which is removed when the build succeeds.
                          A directory given here is never removed.
  --keep-sources          Keep the temporary source directory
  --bde-version=TAG       bde git tag (default: ${BDE_VERSION})
  --bde-tools-version=TAG bde-tools git tag (default: same as --bde-version)
  --rmqcpp-repo=URL       rmqcpp repository (default: ${RMQCPP_REPO})
  --rmqcpp-ref=REF        rmqcpp commit, tag, or branch (default: ${RMQCPP_REF})
  --no-compression        Build rmqcpp without zstd compression support.  Use
                          this if the platform has no zstd CMake config
                          package, since rmqcpp requires find_package(zstd
                          CONFIG).
  --openssl-root=DIR      Value for OPENSSL_ROOT_DIR (default: on macOS,
                          \$(brew --prefix openssl@3) when that formula is
                          installed; otherwise left to CMake)
  --extra-prefix-path=P   Appended to CMAKE_PREFIX_PATH (default: on macOS,
                          \$(brew --prefix); otherwise empty)
  --verify-only           Check that an existing install under --prefix is
                          complete, then exit without building anything
  -h, --help              Show this message

Every option can also be given as an environment variable: PREFIX, JOBS,
BUILD_TYPE, GENERATOR, SRC_DIR, KEEP_SOURCES, BDE_VERSION, BDE_TOOLS_VERSION,
RMQCPP_REPO, RMQCPP_REF, COMPRESSION, OPENSSL_ROOT, EXTRA_PREFIX_PATH,
VERIFY_ONLY.

Required system packages
  Debian/Ubuntu:
      build-essential cmake ninja-build git pkg-config \\
      libboost-dev libssl-dev zlib1g-dev libzstd-dev
  macOS (Homebrew):
      brew install cmake ninja boost openssl@3 zstd

  CMake ${REQUIRED_CMAKE_VERSION} or newer is required (bde needs 3.24, rmqcpp needs 3.25).
  Some still-supported distributions package an older CMake than that --
  Ubuntu 22.04 is at 3.22 -- in which case install a newer one from Kitware's
  APT repository (https://apt.kitware.com) or with "pip install cmake".
  rmqcpp needs only the Boost headers, but dripline-cpp also links
  Boost.Filesystem, so a full Boost installation is the simpler choice.

On success the install prefix gets a ${DEPS_ENV_NAME} file recording the
CMAKE_PREFIX_PATH and OPENSSL_ROOT_DIR that dripline-cpp should be configured
with, so that it resolves the same OpenSSL that rmqcpp was built against.

Note that dripline-cpp itself additionally needs yaml-cpp and RapidJSON, which
are not handled here because they are available as system packages everywhere.
EOF
}

usage_error()
{
    echo "Error: $*" >&2
    echo "Run \"$( basename "$0" ) --help\" for usage." >&2
    exit 1
}

# require_value <option> <number of remaining arguments>
#
# Checks that a space-separated option was actually given a value, so that e.g. a trailing
# "--prefix" produces a usage error instead of an "unbound variable" abort under "set -u".
require_value()
{
    [ "$2" -ge 2 ] || usage_error "option \"$1\" requires a value"
}

###############################################################################
# Argument parsing
###############################################################################

while [ $# -gt 0 ]; do
    # accept both "--opt value" and "--opt=value"
    case "$1" in
        *=*) ARG_VALUE="${1#*=}" ;;
        *)   ARG_VALUE="" ;;
    esac

    case "$1" in
        --prefix=*)            PREFIX="${ARG_VALUE}" ;;
        --prefix)              require_value "$1" $#; PREFIX="$2"; shift ;;
        --jobs=*)              JOBS="${ARG_VALUE}" ;;
        --jobs)                require_value "$1" $#; JOBS="$2"; shift ;;
        --build-type=*)        BUILD_TYPE="${ARG_VALUE}" ;;
        --build-type)          require_value "$1" $#; BUILD_TYPE="$2"; shift ;;
        --generator=*)         GENERATOR="${ARG_VALUE}" ;;
        --generator)           require_value "$1" $#; GENERATOR="$2"; shift ;;
        --src-dir=*)           SRC_DIR="${ARG_VALUE}" ;;
        --src-dir)             require_value "$1" $#; SRC_DIR="$2"; shift ;;
        --keep-sources)        KEEP_SOURCES=TRUE ;;
        --bde-version=*)       BDE_VERSION="${ARG_VALUE}" ;;
        --bde-version)         require_value "$1" $#; BDE_VERSION="$2"; shift ;;
        --bde-tools-version=*) BDE_TOOLS_VERSION="${ARG_VALUE}" ;;
        --bde-tools-version)   require_value "$1" $#; BDE_TOOLS_VERSION="$2"; shift ;;
        --rmqcpp-repo=*)       RMQCPP_REPO="${ARG_VALUE}" ;;
        --rmqcpp-repo)         require_value "$1" $#; RMQCPP_REPO="$2"; shift ;;
        --rmqcpp-ref=*)        RMQCPP_REF="${ARG_VALUE}" ;;
        --rmqcpp-ref)          require_value "$1" $#; RMQCPP_REF="$2"; shift ;;
        --no-compression)      COMPRESSION=OFF ;;
        --openssl-root=*)      OPENSSL_ROOT="${ARG_VALUE}" ;;
        --openssl-root)        require_value "$1" $#; OPENSSL_ROOT="$2"; shift ;;
        --extra-prefix-path=*) EXTRA_PREFIX_PATH="${ARG_VALUE}" ;;
        --extra-prefix-path)   require_value "$1" $#; EXTRA_PREFIX_PATH="$2"; shift ;;
        --verify-only)         VERIFY_ONLY=TRUE ;;
        -h|--help)             usage; exit 0 ;;
        *)                     usage_error "unrecognized option \"$1\"" ;;
    esac
    shift
done

[ -n "${BDE_TOOLS_VERSION}" ] || BDE_TOOLS_VERSION="${BDE_VERSION}"

###############################################################################
# Helpers
###############################################################################

info()
{
    echo
    echo "==> $*"
}

run()
{
    local arg
    printf '+'
    for arg in "$@"; do
        printf ' %q' "${arg}"
    done
    printf '\n'
    "$@"
}

# has_sized_deallocation [extra compiler flags]
#
# Succeeds if the C++ compiler defines __cpp_sized_deallocation when compiling
# with -std=c++17 and any flags given here.
has_sized_deallocation()
{
    local probe="${SRC_DIR}/sized-deallocation-probe.cc"
    printf '%s\n' \
        '#if !defined(__cpp_sized_deallocation)' \
        '#  error __cpp_sized_deallocation is not defined' \
        '#endif' \
        'int main() { return 0; }' > "${probe}"
    "${CXX:-c++}" -std=c++17 "$@" -fsyntax-only "${probe}" > /dev/null 2>&1
}

# verify_install
#
# Checks that everything the dripline-cpp build needs is present under PREFIX.  A wrong
# CMAKE_INSTALL_LIBDIR, an install that quietly skipped a component, or a partially restored CI
# cache shows up here rather than much later in the dripline-cpp build.
verify_install()
{
    local f
    local n_missing=0

    for f in \
        "${PREFIX}/${DEPS_ENV_NAME}" \
        "${PREFIX}/lib/cmake/bsl/bslConfig.cmake" \
        "${PREFIX}/lib/cmake/bdl/bdlConfig.cmake" \
        "${PREFIX}/lib/cmake/bal/balConfig.cmake" \
        "${PREFIX}/lib/libbsl.a" \
        "${PREFIX}/lib/librmq.a" \
        "${PREFIX}/share/rmqcpp/rmqcppConfig.cmake" \
        "${PREFIX}/share/rmqcpp/rmqcppTargets.cmake" \
        "${PREFIX}/include/rmqa_rabbitcontext.h" \
        "${PREFIX}/include/bsl_memory.h"
    do
        if [ -e "${f}" ]; then
            echo "    ok      ${f}"
        else
            echo "    MISSING ${f}"
            n_missing=$(( n_missing + 1 ))
        fi
    done

    if [ "${n_missing}" -ne 0 ]; then
        echo "Error: ${n_missing} expected file(s) are not installed under ${PREFIX}" >&2
        return 1
    fi
}

# write_deps_env
#
# Records what the dripline-cpp build needs to be told about this install.  It lives inside the
# prefix so that it travels with the prefix, including when the prefix is restored from a cache.
write_deps_env()
{
    cat > "${PREFIX}/${DEPS_ENV_NAME}" <<EOF
# Generated by install_dependencies.sh -- source this, then pass the values to CMake:
#     . ${PREFIX}/${DEPS_ENV_NAME}
#     cmake -S . -B build -DCMAKE_PREFIX_PATH="\${DRIPLINE_DEPS_CMAKE_PREFIX_PATH}"
DRIPLINE_DEPS_CMAKE_PREFIX_PATH='${CMAKE_PREFIX_PATH_ARG}'
DRIPLINE_DEPS_OPENSSL_ROOT_DIR='${OPENSSL_ROOT}'
EOF
}

# fetch_repo <url> <ref> <destination>
#
# Shallow-fetches a single ref, which may be a branch, a tag, or a commit SHA.
# Fetching a SHA directly requires the server to allow it; GitHub does.
fetch_repo()
{
    local repo="$1"
    local ref="$2"
    local dest="$3"

    # start from scratch so that re-running with an explicit --src-dir works
    run rm -rf "${dest}"
    run mkdir -p "${dest}"
    run git -C "${dest}" init --quiet
    run git -C "${dest}" remote add origin "${repo}"
    run git -C "${dest}" fetch --quiet --depth 1 origin "${ref}"
    run git -C "${dest}" checkout --quiet --detach FETCH_HEAD
    echo "    ${repo} @ ${ref} -> $( git -C "${dest}" rev-parse HEAD )"
}

###############################################################################
# --verify-only
#
# Checks an existing install and exits.  Nothing is fetched, built, or created.
###############################################################################

if [ "${VERIFY_ONLY}" = "TRUE" ]; then
    [ -d "${PREFIX}" ] || usage_error "prefix \"${PREFIX}\" does not exist"
    PREFIX="$( cd "${PREFIX}" && pwd )"

    info "Verifying the installation in ${PREFIX}"
    verify_install
    echo
    echo "The installation is complete."
    exit 0
fi

###############################################################################
# Setup
###############################################################################

if [ -z "${JOBS}" ]; then
    if command -v nproc > /dev/null 2>&1; then
        JOBS="$( nproc )"
    elif command -v sysctl > /dev/null 2>&1; then
        JOBS="$( sysctl -n hw.ncpu )"
    else
        JOBS=2
    fi
fi

# Optional cmake arguments are collected in arrays, which are expanded with the
# "${array[@]+...}" guard so that an empty array is not an error under "set -u"
# in the bash 3.2 that macOS ships.
GENERATOR_ARGS=()
if [ -n "${GENERATOR}" ]; then
    GENERATOR_ARGS=( -G "${GENERATOR}" )
elif command -v ninja > /dev/null 2>&1; then
    GENERATOR_ARGS=( -G Ninja )
    GENERATOR=Ninja
else
    GENERATOR="(CMake default)"
fi

# Homebrew installs outside of CMake's default search path, so point CMake at it
if [ "$( uname -s )" = "Darwin" ] && command -v brew > /dev/null 2>&1; then
    [ -n "${EXTRA_PREFIX_PATH}" ] || EXTRA_PREFIX_PATH="$( brew --prefix )"
    # "brew --prefix openssl@3" exits 0 and prints a path even when the formula is not installed,
    # which would hand CMake an OPENSSL_ROOT_DIR that does not exist.  FindOpenSSL treats that as a
    # hint rather than a constraint, so it would silently fall back to some other OpenSSL.
    # "--installed" makes brew exit non-zero instead.
    if [ -z "${OPENSSL_ROOT}" ]; then
        if brew --prefix --installed openssl@3 > /dev/null 2>&1; then
            OPENSSL_ROOT="$( brew --prefix --installed openssl@3 )"
        else
            echo "Note: openssl@3 is not installed; leaving the OpenSSL choice to CMake."
            echo "      Pass --openssl-root=DIR to pin it."
        fi
    fi
fi

# Resolve the prefix to an absolute path, since it is passed to CMake from
# several different working directories
mkdir -p "${PREFIX}"
PREFIX="$( cd "${PREFIX}" && pwd )"

CMAKE_PREFIX_PATH_ARG="${PREFIX}"
[ -z "${EXTRA_PREFIX_PATH}" ] || CMAKE_PREFIX_PATH_ARG="${PREFIX};${EXTRA_PREFIX_PATH}"

OPENSSL_ARGS=()
[ -z "${OPENSSL_ROOT}" ] || OPENSSL_ARGS=( "-DOPENSSL_ROOT_DIR=${OPENSSL_ROOT}" )

REMOVE_SRC_DIR=FALSE
if [ -z "${SRC_DIR}" ]; then
    SRC_DIR="$( mktemp -d "${TMPDIR:-/tmp}/dripline-deps.XXXXXXXX" )"
    [ "${KEEP_SOURCES}" = "TRUE" ] || REMOVE_SRC_DIR=TRUE
fi
mkdir -p "${SRC_DIR}"
SRC_DIR="$( cd "${SRC_DIR}" && pwd )"

cat <<EOF

dripline-cpp dependency build
  install prefix:   ${PREFIX}
  source directory: ${SRC_DIR}
  build type:       ${BUILD_TYPE}
  generator:        ${GENERATOR}
  parallel jobs:    ${JOBS}
  bde:              ${BDE_VERSION}
  bde-tools:        ${BDE_TOOLS_VERSION}
  rmqcpp:           ${RMQCPP_REF}
  zstd compression: ${COMPRESSION}
  OpenSSL root:     ${OPENSSL_ROOT:-(left to CMake)}
EOF

###############################################################################
# Preflight
#
# Everything checked here would otherwise fail much later: a too-old CMake fails partway into
# bde's configure, and a missing zstd CMake package fails in rmqcpp's configure, which is step
# 3/3 and so lands only after the long bde build.
###############################################################################

info "Checking the build environment"

command -v cmake > /dev/null 2>&1 || usage_error "cmake was not found on PATH"
command -v git > /dev/null 2>&1 || usage_error "git was not found on PATH"

CMAKE_VERSION="$( cmake --version | head -1 | awk '{ print $3 }' )"
if [ "$( printf '%s\n%s\n' "${REQUIRED_CMAKE_VERSION}" "${CMAKE_VERSION}" | sort -V | head -1 )" \
        != "${REQUIRED_CMAKE_VERSION}" ]; then
    echo "Error: CMake ${REQUIRED_CMAKE_VERSION} or newer is required, but ${CMAKE_VERSION} was found" >&2
    echo "Install a newer CMake from https://apt.kitware.com or with \"pip install cmake\"." >&2
    exit 1
fi
echo "    ok      cmake ${CMAKE_VERSION}"

printf '%s\n' 'int main() { return 0; }' > "${SRC_DIR}/cxx17-probe.cc"
if ! "${CXX:-c++}" -std=c++17 -fsyntax-only "${SRC_DIR}/cxx17-probe.cc" > /dev/null 2>&1; then
    echo "Error: \"${CXX:-c++}\" cannot compile C++17" >&2
    exit 1
fi
echo "    ok      ${CXX:-c++} accepts -std=c++17"

# rmqcpp uses find_package(zstd CONFIG), so a zstd without a CMake config package is not enough.
# Probing with CMake itself is the only way to answer this the same way rmqcpp will.
if [ "${COMPRESSION}" = "ON" ]; then
    mkdir -p "${SRC_DIR}/zstd-probe"
    cat > "${SRC_DIR}/zstd-probe/CMakeLists.txt" <<EOF
cmake_minimum_required( VERSION ${REQUIRED_CMAKE_VERSION} )
# CXX rather than NONE: zstdConfig.cmake does find_dependency( Threads ), and FindThreads
# requires that either C or CXX be enabled
project( zstd_probe LANGUAGES CXX )
find_package( zstd CONFIG REQUIRED )
EOF
    if cmake -S "${SRC_DIR}/zstd-probe" -B "${SRC_DIR}/zstd-probe-build" \
            "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_ARG}" > /dev/null 2>&1; then
        echo "    ok      zstd CMake config package"
    else
        echo "Error: find_package(zstd CONFIG) failed, which rmqcpp requires when compression is on" >&2
        echo "Install a zstd that ships a CMake config package, or re-run with --no-compression." >&2
        exit 1
    fi
fi

###############################################################################
# 1/3: bde-tools
#
# bde-tools is not built or installed.  It provides the BdeBuildSystem CMake
# package that bde's own CMakeLists.txt requires.
###############################################################################

info "[1/3] bde-tools ${BDE_TOOLS_VERSION} (source only)"

fetch_repo "${BDE_TOOLS_REPO}" "${BDE_TOOLS_VERSION}" "${SRC_DIR}/bde-tools"

BDE_BUILD_SYSTEM_DIR="${SRC_DIR}/bde-tools/BdeBuildSystem"
if [ ! -f "${BDE_BUILD_SYSTEM_DIR}/BdeBuildSystemConfig.cmake" ]; then
    echo "Error: ${BDE_BUILD_SYSTEM_DIR}/BdeBuildSystemConfig.cmake not found" >&2
    exit 1
fi

###############################################################################
# 2/3: bde
#
# Notes on the configuration:
#   BBS_BUILD_SYSTEM=1          keeps bde's thousands of test drivers out of
#                               the "all" target; by far the largest influence
#                               on build time
#   BDE_USE_EXTERNAL_PCRE2      deliberately NOT set, so bde builds its
#                               vendored thirdparty/pcre2 and installs a
#                               matching pcre2 CMake package.  Setting it makes
#                               bdlConfig.cmake depend on a "libpcre2-8"
#                               package that has to be shimmed into existence.
#   CMAKE_POSITION_INDEPENDENT_CODE
#                               bde is static-only and gets linked into the
#                               shared libDripline, so it must be PIC
#   CMAKE_INSTALL_LIBDIR        bde-tools uses this without including
#                               GNUInstallDirs, so it has to be set explicitly
###############################################################################

info "[2/3] bde ${BDE_VERSION} -> ${PREFIX}"

fetch_repo "${BDE_REPO}" "${BDE_VERSION}" "${SRC_DIR}/bde"

# bde's C++17 conformance check in bsls_compilerfeatures.cpp requires any Clang that reports
# version 19 or newer to define __cpp_sized_deallocation.  Apple's clang reports such a version
# (its version numbers are not upstream clang's) but still does not enable sized deallocation by
# default, so the flag has to be added by hand.  The macro is not referenced by any bde header,
# so this affects nothing but the bde build itself.
BDE_CXX_FLAGS="${BDE_CXX_FLAGS:-}"
if ! has_sized_deallocation && has_sized_deallocation -fsized-deallocation; then
    echo "    adding -fsized-deallocation: the compiler needs it to define __cpp_sized_deallocation"
    BDE_CXX_FLAGS="${BDE_CXX_FLAGS} -fsized-deallocation"
fi

BDE_FLAG_ARGS=()
[ -z "${BDE_CXX_FLAGS}" ] || BDE_FLAG_ARGS=( "-DCMAKE_CXX_FLAGS=${BDE_CXX_FLAGS}" )

run cmake -S "${SRC_DIR}/bde" -B "${SRC_DIR}/bde-build" \
    ${BDE_FLAG_ARGS[@]+"${BDE_FLAG_ARGS[@]}"} \
    ${GENERATOR_ARGS[@]+"${GENERATOR_ARGS[@]}"} \
    "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}" \
    "-DCMAKE_INSTALL_PREFIX=${PREFIX}" \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_CXX_EXTENSIONS=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DBDE_BUILD_TARGET_CPP17=ON \
    -DBBS_BUILD_SYSTEM=1 \
    "-DBdeBuildSystem_DIR=${BDE_BUILD_SYSTEM_DIR}"

run cmake --build "${SRC_DIR}/bde-build" -j "${JOBS}"
run cmake --install "${SRC_DIR}/bde-build"

###############################################################################
# 3/3: rmqcpp
#
# Testing is disabled both to skip the GTest requirement and to avoid the
# configure-time "pip install" in src/tests/integration/CMakeLists.txt.
###############################################################################

info "[3/3] rmqcpp ${RMQCPP_REF} -> ${PREFIX}"

fetch_repo "${RMQCPP_REPO}" "${RMQCPP_REF}" "${SRC_DIR}/rmqcpp"

run cmake -S "${SRC_DIR}/rmqcpp" -B "${SRC_DIR}/rmqcpp-build" \
    ${GENERATOR_ARGS[@]+"${GENERATOR_ARGS[@]}"} \
    "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}" \
    "-DCMAKE_INSTALL_PREFIX=${PREFIX}" \
    -DCMAKE_INSTALL_LIBDIR=lib \
    "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_ARG}" \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBDE_BUILD_TARGET_CPP17=ON \
    -Drmqcpp_ENABLE_TESTING=OFF \
    -Drmqcpp_BUILD_EXAMPLES=OFF \
    "-DENABLE_COMPRESSION=${COMPRESSION}" \
    ${OPENSSL_ARGS[@]+"${OPENSSL_ARGS[@]}"}

run cmake --build "${SRC_DIR}/rmqcpp-build" -j "${JOBS}"
run cmake --install "${SRC_DIR}/rmqcpp-build"

###############################################################################
# Verification
###############################################################################

info "Verifying the installation"

write_deps_env

if ! verify_install; then
    echo "The source and build trees were left in ${SRC_DIR}" >&2
    exit 1
fi

###############################################################################
# Cleanup
###############################################################################

if [ "${REMOVE_SRC_DIR}" = "TRUE" ]; then
    info "Removing ${SRC_DIR}"
    run rm -rf "${SRC_DIR}"
else
    info "Sources and build trees left in ${SRC_DIR}"
fi

cat <<EOF

Done.  Configure dripline-cpp with:
    cmake -S . -B build -DCMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH_ARG}"${OPENSSL_ROOT:+ \\
        -DOPENSSL_ROOT_DIR="${OPENSSL_ROOT}"}

These values were also written to ${PREFIX}/${DEPS_ENV_NAME}:
    . ${PREFIX}/${DEPS_ENV_NAME}
EOF
