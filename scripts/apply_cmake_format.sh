#!/bin/bash
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

if ! command -v cmake-format &> /dev/null; then
    echo "Error: cmake-format is not installed."
    echo "Install it with: pip install cmakelang==0.6.13"
    exit 1
fi

echo "Applying cmake-format to all CMake files..."
find . -type f \( -name "CMakeLists.txt" -o -name "*.cmake" -o -name "*.cmake.in" \) -not -path "*/build/*" -not -path "*/.git/*" -not -path "*/.vscode/*" -not -path "*/docs/html/*" -exec cmake-format -i {} +
echo "CMake formatting complete."
