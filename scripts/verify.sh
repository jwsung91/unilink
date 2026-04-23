#!/bin/bash
set -e
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

echo "🔍 Step 1: Formatting code..."
./scripts/apply_clang_format.sh

echo -e "\n🛠️ Step 2: Building project (Debug)..."
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

echo -e "\n🧪 Step 3: Running unit tests..."
ctest -j$(nproc) -L unit --output-on-failure

echo -e "\n✅ [SUCCESS] Project is verified and ready!"
