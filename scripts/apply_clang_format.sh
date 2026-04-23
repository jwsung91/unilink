#!/bin/bash
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed."
    exit 1
fi
echo "Applying clang-format to all C++ files..."
find . -type f \( -name "*.h" -o -name "*.hpp" -o -name "*.c" -o -name "*.cpp" -o -name "*.cc" -o -name "*.in" \) -not -path "*/build/*" -not -path "*/.git/*" -not -path "*/.vscode/*" -not -path "*/docs/html/*" -not -path "*/cmake/*.cmake.in" -not -path "*/package/*.pc.in" -exec clang-format -i -style=file {} +
echo "✅ Code formatting complete."
