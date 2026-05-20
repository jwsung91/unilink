#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CALLER_PWD="$(pwd)"

BUILD_DIR="${BUILD_DIR:-${PROJECT_ROOT}/build/consumer-smoke}"
INSTALL_PREFIX="${INSTALL_PREFIX:-${PROJECT_ROOT}/build/consumer-smoke-install}"
CONSUMER_DIR="${CONSUMER_DIR:-${PROJECT_ROOT}/build/consumer-smoke-app}"
LIBRARY_MODE="${LIBRARY_MODE:-shared}"

CMAKE_GENERATOR="${CMAKE_GENERATOR:-Ninja}"
CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"

usage() {
  cat <<'EOF'
Usage: scripts/verify_installed_consumer.sh [options]

Options:
  --build-dir PATH          Build directory for unilink
  --install-prefix PATH     Temporary install prefix
  --consumer-dir PATH       External consumer project directory
  --library-mode MODE       shared, static, or both
  --help                    Show this help message
EOF
}

die() {
  echo "error: $*" >&2
  exit 1
}

log_step() {
  echo
  echo "==> $*"
}

require_arg() {
  local option="$1"
  local value="${2:-}"
  if [[ -z "$value" || "$value" == --* ]]; then
    die "$option requires a value"
  fi
}

make_absolute() {
  local path="$1"
  if [[ "$path" = /* ]]; then
    echo "$path"
  else
    echo "${CALLER_PWD}/${path}"
  fi
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir)
      require_arg "$1" "${2:-}"
      BUILD_DIR="$2"
      shift 2
      ;;
    --install-prefix)
      require_arg "$1" "${2:-}"
      INSTALL_PREFIX="$2"
      shift 2
      ;;
    --consumer-dir)
      require_arg "$1" "${2:-}"
      CONSUMER_DIR="$2"
      shift 2
      ;;
    --library-mode)
      require_arg "$1" "${2:-}"
      LIBRARY_MODE="$2"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

case "$LIBRARY_MODE" in
  shared)
    UNILINK_BUILD_SHARED=ON
    UNILINK_BUILD_STATIC=OFF
    ;;
  static)
    UNILINK_BUILD_SHARED=OFF
    UNILINK_BUILD_STATIC=ON
    ;;
  both)
    UNILINK_BUILD_SHARED=ON
    UNILINK_BUILD_STATIC=ON
    ;;
  *)
    die "invalid --library-mode: $LIBRARY_MODE (expected shared, static, or both)"
    ;;
esac

BUILD_DIR="$(make_absolute "$BUILD_DIR")"
INSTALL_PREFIX="$(make_absolute "$INSTALL_PREFIX")"
CONSUMER_DIR="$(make_absolute "$CONSUMER_DIR")"

log_step "Preparing directories"
echo "Project root:     $PROJECT_ROOT"
echo "Build dir:        $BUILD_DIR"
echo "Install prefix:   $INSTALL_PREFIX"
echo "Consumer dir:     $CONSUMER_DIR"
echo "Library mode:     $LIBRARY_MODE"
echo "CMake generator:  $CMAKE_GENERATOR"
echo "CMake build type: $CMAKE_BUILD_TYPE"

rm -rf "$BUILD_DIR" "$INSTALL_PREFIX" "$CONSUMER_DIR"
mkdir -p "$BUILD_DIR" "$INSTALL_PREFIX" "$CONSUMER_DIR"

configure_args=(
  -S "$PROJECT_ROOT"
  -B "$BUILD_DIR"
  -G "$CMAKE_GENERATOR"
  -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
  -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
  -DUNILINK_BUILD_SHARED="$UNILINK_BUILD_SHARED"
  -DUNILINK_BUILD_STATIC="$UNILINK_BUILD_STATIC"
  -DUNILINK_BUILD_TESTS=OFF
  -DUNILINK_BUILD_DOCS=OFF
  -DUNILINK_ENABLE_INSTALL=ON
  -DUNILINK_ENABLE_PKGCONFIG=ON
  -DUNILINK_ENABLE_EXPORT_HEADER=ON
)

if [[ -n "${CMAKE_TOOLCHAIN_FILE:-}" ]]; then
  configure_args+=("-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
fi

if [[ -n "${VCPKG_TARGET_TRIPLET:-}" ]]; then
  configure_args+=("-DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}")
fi

log_step "Configuring unilink"
cmake "${configure_args[@]}"

log_step "Building unilink"
cmake --build "$BUILD_DIR" --parallel

log_step "Installing unilink"
cmake --install "$BUILD_DIR"

log_step "Generating external consumer project"
cat > "$CONSUMER_DIR/CMakeLists.txt" <<'EOF'
cmake_minimum_required(VERSION 3.12)
project(unilink_consumer_smoke LANGUAGES CXX)

find_package(unilink CONFIG REQUIRED)

add_executable(unilink_consumer_smoke main.cpp)
target_link_libraries(unilink_consumer_smoke PRIVATE unilink::unilink)
target_compile_features(unilink_consumer_smoke PRIVATE cxx_std_20)
EOF

cat > "$CONSUMER_DIR/main.cpp" <<'EOF'
#include <cstdint>
#include <memory>
#include <string>

#include <unilink/unilink.hpp>

int main() {
    auto tcp_client = unilink::tcp_client("127.0.0.1", 8080).build();
    auto tcp_server = unilink::tcp_server(8080).build();

    auto udp_client = unilink::udp_client(0)
        .remote("127.0.0.1", 9000)
        .build();

    auto udp_server = unilink::udp_server(9000).build();

#ifndef _WIN32
    auto serial = unilink::serial("/dev/null", 115200).build();
    auto uds_client = unilink::uds_client("/tmp/unilink-consumer-smoke.sock").build();
    auto uds_server = unilink::uds_server("/tmp/unilink-consumer-smoke.sock").build();

    return (tcp_client && tcp_server && udp_client && udp_server &&
            serial && uds_client && uds_server) ? 0 : 1;
#else
    return (tcp_client && tcp_server && udp_client && udp_server) ? 0 : 1;
#endif
}
EOF

consumer_build_dir="$CONSUMER_DIR/build"

consumer_args=(
  -S "$CONSUMER_DIR"
  -B "$consumer_build_dir"
  -G "$CMAKE_GENERATOR"
  -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
  -DCMAKE_PREFIX_PATH="$INSTALL_PREFIX"
)

if [[ -n "${CMAKE_TOOLCHAIN_FILE:-}" ]]; then
  consumer_args+=("-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
fi

if [[ -n "${VCPKG_TARGET_TRIPLET:-}" ]]; then
  consumer_args+=("-DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}")
fi

log_step "Configuring external consumer"
cmake "${consumer_args[@]}"

log_step "Building external consumer"
cmake --build "$consumer_build_dir" --parallel

echo
echo "Installed consumer smoke passed for library mode: $LIBRARY_MODE"
