#!/bin/bash
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

job_count() {
  if command -v nproc >/dev/null 2>&1; then
    nproc
  elif command -v sysctl >/dev/null 2>&1; then
    sysctl -n hw.ncpu
  else
    echo 2
  fi
}

JOBS="${JOBS:-$(job_count)}"

if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed."
    exit 1
fi
echo "Applying clang-format to all C++ files using ${JOBS:-2} jobs..."
find . -type f \( -name "*.h" -o -name "*.hpp" -o -name "*.c" -o -name "*.cpp" -o -name "*.cc" -o -name "*.in" \) \
    -not -path "*/build/*" \
    -not -path "*/vcpkg/*" \
    -not -path "*/.git/*" \
    -not -path "*/.vscode/*" \
    -not -path "*/docs/html/*" \
    -not -path "*/cmake/*.cmake.in" \
    -not -path "*/cmake/*.pc.in" \
    -print0 | xargs -0 -P "${JOBS:-2}" -n 20 clang-format -i -style=file
echo "✅ Code formatting complete."
