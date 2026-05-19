#!/bin/bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

section() { echo -e "\n===== $* ====="; }

job_count() {
  if command -v nproc >/dev/null 2>&1; then
    nproc
  elif command -v sysctl >/dev/null 2>&1; then
    sysctl -n hw.ncpu
  else
    echo 2
  fi
}

JOBS="$(job_count)"
export JOBS

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
SKIP_FORMAT=0
SKIP_DOCS=0

for arg in "$@"; do
  case "$arg" in
    --tests-only|-t)
      SKIP_FORMAT=1
      SKIP_DOCS=1
      ;;
    --skip-format)
      SKIP_FORMAT=1
      ;;
    --skip-docs)
      SKIP_DOCS=1
      ;;
    --help|-h)
      echo "Usage: $0 [options]"
      echo ""
      echo "Options:"
      echo "  --tests-only, -t   Skip formatting and doc snippets; run build + tests only"
      echo "  --skip-format      Skip clang-format / cmake-format step"
      echo "  --skip-docs        Skip documentation snippet compilation"
      echo "  --help, -h         Show this help message"
      exit 0
      ;;
    *)
      echo "Unknown option: $arg" >&2
      exit 1
      ;;
  esac
done

UNILINK_VERIFY_PRESET_USER_SET=1
if [[ -z "${UNILINK_VERIFY_PRESET:-}" ]]; then
  UNILINK_VERIFY_PRESET_USER_SET=0
  # Detect host platform and suggest a preset
  OS="$(uname -s)"
  ARCH="$(uname -m)"
  case "${OS}:${ARCH}" in
    Linux:x86_64)  UNILINK_VERIFY_PRESET="dev-linux-x64" ;;
    Linux:aarch64 | Linux:arm64) UNILINK_VERIFY_PRESET="dev-linux-arm64" ;;
    Darwin:arm64)  UNILINK_VERIFY_PRESET="dev-macos-arm64" ;;
    Darwin:x86_64) UNILINK_VERIFY_PRESET="dev-macos-x64" ;;
  esac
fi

UNILINK_VERIFY_BUILD_DIR_USER_SET=1
if [[ -z "${UNILINK_VERIFY_BUILD_DIR:-}" ]]; then
  UNILINK_VERIFY_BUILD_DIR_USER_SET=0
  if [[ -n "${UNILINK_VERIFY_PRESET:-}" ]]; then
    UNILINK_VERIFY_BUILD_DIR="build/${UNILINK_VERIFY_PRESET}"
  else
    UNILINK_VERIFY_BUILD_DIR="build"
  fi
fi

if [[ -n "${UNILINK_VERIFY_PRESET:-}" ]]; then
  VCPKG_TOOLCHAIN="${PROJECT_ROOT}/vcpkg/scripts/buildsystems/vcpkg.cmake"
  PRESET_UNAVAILABLE_REASON=""
  if [[ ! -f "${VCPKG_TOOLCHAIN}" ]]; then
    PRESET_UNAVAILABLE_REASON="missing local vcpkg toolchain: ${VCPKG_TOOLCHAIN}"
  elif ! command -v ninja >/dev/null 2>&1; then
    PRESET_UNAVAILABLE_REASON="missing Ninja build tool required by CMakePresets.json"
  fi

  if [[ -n "${PRESET_UNAVAILABLE_REASON}" ]]; then
    if [[ "${UNILINK_VERIFY_PRESET_USER_SET}" -eq 1 ]]; then
      echo "Requested preset '${UNILINK_VERIFY_PRESET}' is unavailable: ${PRESET_UNAVAILABLE_REASON}" >&2
      echo "Run ./scripts/setup_dev_env.sh or choose a non-preset build directory." >&2
      exit 1
    fi

    echo "Auto-detected preset '${UNILINK_VERIFY_PRESET}' is unavailable: ${PRESET_UNAVAILABLE_REASON}"
    UNILINK_VERIFY_PRESET=""
    if [[ "${UNILINK_VERIFY_BUILD_DIR_USER_SET}" -eq 0 ]]; then
      UNILINK_VERIFY_BUILD_DIR="build/verify"
    fi
    echo "Falling back to direct CMake configure in ${UNILINK_VERIFY_BUILD_DIR}."
  fi
fi

# ---------------------------------------------------------------------------
# Step 1: Format
# ---------------------------------------------------------------------------
if [[ "${SKIP_FORMAT}" -eq 0 ]]; then
  section "Step 1: Formatting code"
  ./scripts/apply_clang_format.sh &
  CLANG_PID=$!
  ./scripts/apply_cmake_format.sh &
  CMAKE_PID=$!

  # Wait for both and check for failures
  wait "$CLANG_PID" || { echo "clang-format failed"; exit 1; }
  wait "$CMAKE_PID" || { echo "cmake-format failed"; exit 1; }
else
  section "Step 1: Formatting code [SKIPPED]"
fi

# ---------------------------------------------------------------------------
# Step 2: Build library
# ---------------------------------------------------------------------------
section "Step 2: Building project (Debug)"
if [[ -n "${UNILINK_VERIFY_PRESET:-}" ]]; then
  echo "Using preset: ${UNILINK_VERIFY_PRESET}"
  cmake --preset "${UNILINK_VERIFY_PRESET}"
else
  mkdir -p "${UNILINK_VERIFY_BUILD_DIR}"
  # Check if local vcpkg toolchain exists and use it as fallback
  VCPKG_TOOLCHAIN="${PROJECT_ROOT}/vcpkg/scripts/buildsystems/vcpkg.cmake"
  if [[ -f "${VCPKG_TOOLCHAIN}" ]]; then
    echo "Using local vcpkg toolchain: ${VCPKG_TOOLCHAIN}"
    cmake -S . -B "${UNILINK_VERIFY_BUILD_DIR}" \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_STANDARD=20 \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DUNILINK_BUILD_TESTS=ON \
      -DUNILINK_ENABLE_CONFIG=ON \
      -DCMAKE_TOOLCHAIN_FILE="${VCPKG_TOOLCHAIN}"
  else
    cmake -S . -B "${UNILINK_VERIFY_BUILD_DIR}" \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_STANDARD=20 \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DUNILINK_BUILD_TESTS=ON \
      -DUNILINK_ENABLE_CONFIG=ON
  fi
fi
cmake --build "${UNILINK_VERIFY_BUILD_DIR}" -j"${JOBS}"

# ---------------------------------------------------------------------------
# Step 3: Compile documentation snippets
# ---------------------------------------------------------------------------
if [[ "${SKIP_DOCS}" -eq 0 ]]; then
  section "Step 3: Compiling documentation snippets"
  cmake --build "${UNILINK_VERIFY_BUILD_DIR}" -j"${JOBS}" --target doc_snippets_smoke
else
  section "Step 3: Compiling documentation snippets [SKIPPED]"
fi

# ---------------------------------------------------------------------------
# Step 4: Full test suite
# ---------------------------------------------------------------------------
section "Step 4: Running full test suite"
ctest --test-dir "${UNILINK_VERIFY_BUILD_DIR}" -j"${JOBS}" --output-on-failure

echo ""
echo "===== [SUCCESS] All checks passed! ====="
