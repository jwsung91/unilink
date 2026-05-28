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

if ! command -v cmake-format &> /dev/null; then
    echo "Error: cmake-format is not installed."
    echo "Install it with: pip install cmakelang==0.6.13"
    exit 1
fi

echo "Applying cmake-format to all CMake files using ${JOBS:-2} jobs..."
find . -type f \( -name "CMakeLists.txt" -o -name "*.cmake" -o -name "*.cmake.in" \) \
    -not -path "*/build/*" \
    -not -path "*/vcpkg/*" \
    -not -path "*/.git/*" \
    -not -path "*/.vscode/*" \
    -print0 | xargs -0 -P "${JOBS:-2}" -n 20 cmake-format -i
echo "CMake formatting complete."
