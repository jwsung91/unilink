#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VCPKG_ROOT="${VCPKG_ROOT:-"${PROJECT_ROOT}/vcpkg"}"
VCPKG_PACKAGES="${VCPKG_PACKAGES:-boost-asio boost-system spdlog}"

detect_triplet() {
  local os arch
  os="$(uname -s)"
  arch="$(uname -m)"

  case "${os}:${arch}" in
    Linux:x86_64) echo "x64-linux" ;;
    Linux:aarch64 | Linux:arm64) echo "arm64-linux" ;;
    Darwin:arm64) echo "arm64-osx" ;;
    Darwin:x86_64) echo "x64-osx" ;;
    *)
      echo "Unsupported host ${os}/${arch}. Set VCPKG_TARGET_TRIPLET explicitly." >&2
      return 1
      ;;
  esac
}

VCPKG_TARGET_TRIPLET="${VCPKG_TARGET_TRIPLET:-"$(detect_triplet)"}"

if [ ! -d "${VCPKG_ROOT}/.git" ]; then
  echo "Creating local vcpkg checkout at ${VCPKG_ROOT}"
  git clone --depth 1 https://github.com/microsoft/vcpkg.git "${VCPKG_ROOT}"
fi

"${VCPKG_ROOT}/bootstrap-vcpkg.sh" -disableMetrics

retry_vcpkg_install() {
  local attempt
  for attempt in 1 2 3; do
    if "${VCPKG_ROOT}/vcpkg" install ${VCPKG_PACKAGES} --triplet "${VCPKG_TARGET_TRIPLET}" --clean-after-build; then
      return 0
    fi
    if [ "${attempt}" -eq 3 ]; then
      return 1
    fi
    echo "vcpkg install failed; retrying attempt $((attempt + 1)) of 3..."
    sleep $((attempt * 10))
  done
}

retry_vcpkg_install

cat <<EOF

Development dependencies are ready.

VCPKG_ROOT=${VCPKG_ROOT}
VCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}

By default this script uses the untracked repository-local vcpkg/ checkout.
Delete it any time to reclaim space; rerun this script to recreate it.
Set VCPKG_ROOT to reuse an external vcpkg checkout.

The preset commands below require CMake 3.21+.
Plain non-preset builds remain supported with CMake 3.12+.

Configure with one of:
  cmake --preset dev-linux-x64
  cmake --preset dev-linux-arm64
  cmake --preset dev-macos-arm64
  cmake --preset dev-macos-x64

Then build with:
  cmake --build --preset <preset-name>
EOF
