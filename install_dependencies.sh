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
# bde and bde-tools are released in lockstep and share a tag.  The rmqcpp ref
# must be kept in sync with the rmqcpp_ref build arg in the Dockerfile and the
# RMQCPP_REF value in .github/workflows/publish.yaml.
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
                          \$(brew --prefix openssl@3); otherwise unset)
  --extra-prefix-path=P   Appended to CMAKE_PREFIX_PATH (default: on macOS,
                          \$(brew --prefix); otherwise empty)
  -h, --help              Show this message

Every option can also be given as an environment variable: PREFIX, JOBS,
BUILD_TYPE, GENERATOR, SRC_DIR, KEEP_SOURCES, BDE_VERSION, BDE_TOOLS_VERSION,
RMQCPP_REPO, RMQCPP_REF, COMPRESSION, OPENSSL_ROOT, EXTRA_PREFIX_PATH.

Required system packages
  Debian/Ubuntu:
      build-essential cmake ninja-build git pkg-config \\
      libboost-dev libssl-dev zlib1g-dev libzstd-dev
  macOS (Homebrew):
      brew install cmake ninja boost openssl@3 zstd

  CMake 3.25 or newer is required (bde needs 3.24, rmqcpp needs 3.25).
  rmqcpp needs only the Boost headers, but dripline-cpp also links
  Boost.Filesystem, so a full Boost installation is the simpler choice.

Note that dripline-cpp itself additionally needs yaml-cpp and RapidJSON, which
are not handled here because they are available as system packages everywhere.
EOF
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
        --prefix)              PREFIX="$2"; shift ;;
        --jobs=*)              JOBS="${ARG_VALUE}" ;;
        --jobs)                JOBS="$2"; shift ;;
        --build-type=*)        BUILD_TYPE="${ARG_VALUE}" ;;
        --build-type)          BUILD_TYPE="$2"; shift ;;
        --generator=*)         GENERATOR="${ARG_VALUE}" ;;
        --generator)           GENERATOR="$2"; shift ;;
        --src-dir=*)           SRC_DIR="${ARG_VALUE}" ;;
        --src-dir)             SRC_DIR="$2"; shift ;;
        --keep-sources)        KEEP_SOURCES=TRUE ;;
        --bde-version=*)       BDE_VERSION="${ARG_VALUE}" ;;
        --bde-version)         BDE_VERSION="$2"; shift ;;
        --bde-tools-version=*) BDE_TOOLS_VERSION="${ARG_VALUE}" ;;
        --bde-tools-version)   BDE_TOOLS_VERSION="$2"; shift ;;
        --rmqcpp-repo=*)       RMQCPP_REPO="${ARG_VALUE}" ;;
        --rmqcpp-repo)         RMQCPP_REPO="$2"; shift ;;
        --rmqcpp-ref=*)        RMQCPP_REF="${ARG_VALUE}" ;;
        --rmqcpp-ref)          RMQCPP_REF="$2"; shift ;;
        --no-compression)      COMPRESSION=OFF ;;
        --openssl-root=*)      OPENSSL_ROOT="${ARG_VALUE}" ;;
        --openssl-root)        OPENSSL_ROOT="$2"; shift ;;
        --extra-prefix-path=*) EXTRA_PREFIX_PATH="${ARG_VALUE}" ;;
        --extra-prefix-path)   EXTRA_PREFIX_PATH="$2"; shift ;;
        -h|--help)             usage; exit 0 ;;
        *)
            echo "Error: unrecognized option \"$1\"" >&2
            echo "Run \"$( basename "$0" ) --help\" for usage." >&2
            exit 1
            ;;
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
    if [ -z "${OPENSSL_ROOT}" ] && brew --prefix openssl@3 > /dev/null 2>&1; then
        OPENSSL_ROOT="$( brew --prefix openssl@3 )"
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
EOF

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
#
# A wrong CMAKE_INSTALL_LIBDIR, or an install that quietly skipped a component,
# shows up here rather than much later in the dripline-cpp build.
###############################################################################

info "Verifying the installation"

N_MISSING=0
for f in \
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
        N_MISSING=$(( N_MISSING + 1 ))
    fi
done

if [ "${N_MISSING}" -ne 0 ]; then
    echo "Error: ${N_MISSING} expected file(s) were not installed" >&2
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
    cmake -S . -B build -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_ARG}
EOF
