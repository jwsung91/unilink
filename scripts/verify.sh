#!/bin/bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

section() { echo -e "\n===== $* ====="; }

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
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug 2>&1 | tail -5
cmake --build . -j"$(nproc)" \
  --target unilink_shared unilink_static \
  tcp_echo_server tcp_echo_client tcp_broadcast_server \
  udp_receiver udp_sender \
  uds_echo_server uds_echo_client \
  serial_echo
cd "$PROJECT_ROOT"

# ---------------------------------------------------------------------------
# Step 3: Compile documentation snippets
# ---------------------------------------------------------------------------
section "Step 3: Compiling documentation snippets"
cd build
cmake --build . -j"$(nproc)" --target doc_snippets_smoke
cd "$PROJECT_ROOT"

# ---------------------------------------------------------------------------
# Step 4: Unit tests
# ---------------------------------------------------------------------------
section "Step 4: Running unit tests"
cd build
ctest -j"$(nproc)" -L unit --output-on-failure
cd "$PROJECT_ROOT"

echo ""
echo "===== [SUCCESS] All checks passed! ====="
