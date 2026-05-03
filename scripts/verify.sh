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

if [[ -z "${UNILINK_VERIFY_PRESET:-}" ]]; then
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

if [[ -z "${UNILINK_VERIFY_BUILD_DIR:-}" ]]; then
  if [[ -n "${UNILINK_VERIFY_PRESET:-}" ]]; then
    UNILINK_VERIFY_BUILD_DIR="build/${UNILINK_VERIFY_PRESET}"
  else
    UNILINK_VERIFY_BUILD_DIR="build"
  fi
fi

# ---------------------------------------------------------------------------
# Step 1: Format
# ---------------------------------------------------------------------------
if [[ "${SKIP_FORMAT}" -eq 0 ]]; then
  section "Step 1: Formatting code"
  ./scripts/apply_clang_format.sh
  ./scripts/apply_cmake_format.sh
else
  section "Step 1: Formatting code [SKIPPED]"
fi

# ---------------------------------------------------------------------------
# Step 2: Build library + examples
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
      -DCMAKE_TOOLCHAIN_FILE="${VCPKG_TOOLCHAIN}"
  else
    cmake -S . -B "${UNILINK_VERIFY_BUILD_DIR}" -DCMAKE_BUILD_TYPE=Debug
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
# Step 4: Unit tests
# ---------------------------------------------------------------------------
section "Step 4: Running unit tests"
ctest --test-dir "${UNILINK_VERIFY_BUILD_DIR}" -j"${JOBS}" -L unit --output-on-failure

echo ""
echo "===== [SUCCESS] All checks passed! ====="
