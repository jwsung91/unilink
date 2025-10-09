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
    echo "  macOS: brew install doxygen"
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

echo "📚 Generating documentation..."

# Generate documentation
doxygen docs/config/Doxyfile

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
