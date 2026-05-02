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
section "Step 1: Formatting code"
./scripts/apply_clang_format.sh
./scripts/apply_cmake_format.sh

# ---------------------------------------------------------------------------
# Step 2: Build library + examples
# ---------------------------------------------------------------------------
section "Step 2: Building project (Debug)"
if [[ -n "${UNILINK_VERIFY_PRESET:-}" ]]; then
  cmake --preset "${UNILINK_VERIFY_PRESET}"
else
  mkdir -p "${UNILINK_VERIFY_BUILD_DIR}"
  cmake -S . -B "${UNILINK_VERIFY_BUILD_DIR}" -DCMAKE_BUILD_TYPE=Debug
fi
cmake --build "${UNILINK_VERIFY_BUILD_DIR}" -j"${JOBS}" \
  --target unilink_shared unilink_static \
  sync_tcp_echo_server sync_tcp_echo_client sync_tcp_broadcast_server \
  async_tcp_echo_server async_tcp_echo_client \
  sync_udp_receiver sync_udp_sender \
  async_udp_receiver async_udp_sender \
  sync_uds_echo_server sync_uds_echo_client \
  async_uds_echo_server async_uds_echo_client \
  sync_serial_echo async_serial_echo

# ---------------------------------------------------------------------------
# Step 3: Compile documentation snippets
# ---------------------------------------------------------------------------
section "Step 3: Compiling documentation snippets"
cmake --build "${UNILINK_VERIFY_BUILD_DIR}" -j"${JOBS}" --target doc_snippets_smoke

# ---------------------------------------------------------------------------
# Step 4: Unit tests
# ---------------------------------------------------------------------------
section "Step 4: Running unit tests"
ctest --test-dir "${UNILINK_VERIFY_BUILD_DIR}" -j"${JOBS}" -L unit --output-on-failure

echo ""
echo "===== [SUCCESS] All checks passed! ====="
