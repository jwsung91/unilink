#!/bin/bash

# Generate documentation for unilink library
# This script automates the documentation generation process

set -e  # Exit on any error

echo "🔧 Setting up documentation generation..."

# Check if Doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "❌ Doxygen is not installed!"
    echo "Please install Doxygen first:"
    echo "  Ubuntu/Debian: sudo apt install doxygen"
    echo "  CentOS/RHEL: sudo yum install doxygen"
    echo "  Windows: Download from https://www.doxygen.nl/download.html"
    exit 1
fi

echo "✅ Doxygen found: $(doxygen --version)"

# Create docs directory if it doesn't exist
mkdir -p docs

# Check if Doxyfile exists
if [ ! -f "docs/config/Doxyfile" ]; then
    echo "❌ Doxyfile not found!"
    echo "Please run this script from the project root directory."
    exit 1
fi

if ! command -v python3 &> /dev/null; then
    echo "❌ python3 is required to read the project version from CMakeLists.txt"
    exit 1
fi

PROJECT_VERSION=$(python3 - <<'PY'
import pathlib
import re

cmake_lists = pathlib.Path("CMakeLists.txt")
match = re.search(
    r"^\s*VERSION\s+([0-9]+(?:\.[0-9]+){1,3})",
    cmake_lists.read_text(),
    re.MULTILINE,
)
print(match.group(1) if match else "")
PY
)

if [ -z "$PROJECT_VERSION" ]; then
    echo "⚠️ Could not find project version in CMakeLists.txt, defaulting to 0.0.0"
    PROJECT_VERSION="0.0.0"
fi

echo "📦 Using project version: $PROJECT_VERSION"
echo "📚 Generating documentation..."

# Generate documentation
PROJECT_NUMBER="$PROJECT_VERSION" doxygen docs/config/Doxyfile

# Check if documentation was generated successfully
if [ -d "docs/html" ] && [ -f "docs/html/index.html" ]; then
    echo "✅ Documentation generated successfully!"
    echo "📖 Open docs/html/index.html in your browser to view the documentation"
    echo ""
    echo "📊 Documentation statistics:"
    echo "   - HTML files: $(find docs/html -name "*.html" | wc -l)"
    echo "   - Total size: $(du -sh docs/html | cut -f1)"
    echo ""
    echo "🌐 You can serve the documentation locally with:"
    echo "   cd docs/html && python3 -m http.server 8000"
    echo "   Then open http://localhost:8000 in your browser"
else
    echo "❌ Documentation generation failed!"
    echo "Check the doxygen output above for errors."
    exit 1
fi

echo "🎉 Documentation generation complete!"
