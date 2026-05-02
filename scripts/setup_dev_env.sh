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
  git clone --depth 1 https://github.com/microsoft/vcpkg.git "${VCPKG_ROOT}"
fi

"${VCPKG_ROOT}/bootstrap-vcpkg.sh" -disableMetrics
"${VCPKG_ROOT}/vcpkg" install ${VCPKG_PACKAGES} --triplet "${VCPKG_TARGET_TRIPLET}" --clean-after-build

cat <<EOF

Development dependencies are ready.

VCPKG_ROOT=${VCPKG_ROOT}
VCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}

Configure with one of:
  cmake --preset dev-linux-x64
  cmake --preset dev-linux-arm64
  cmake --preset dev-macos-arm64
  cmake --preset dev-macos-x64

Then build with:
  cmake --build --preset <preset-name>
EOF
