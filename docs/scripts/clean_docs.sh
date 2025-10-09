#!/bin/bash

# Clean generated documentation
# This script removes all generated documentation files

set -e  # Exit on any error

echo "🧹 Cleaning documentation..."

# Remove generated HTML files
if [ -d "docs/html" ]; then
    echo "Removing docs/html/ directory..."
    rm -rf docs/html
fi

# Remove generated LaTeX files
if [ -d "docs/latex" ]; then
    echo "Removing docs/latex/ directory..."
    rm -rf docs/latex
fi

# Remove generated RTF files
if [ -d "docs/rtf" ]; then
    echo "Removing docs/rtf/ directory..."
    rm -rf docs/rtf
fi

# Remove generated XML files
if [ -d "docs/xml" ]; then
    echo "Removing docs/xml/ directory..."
    rm -rf docs/xml
fi

# Remove generated man pages
if [ -d "docs/man" ]; then
    echo "Removing docs/man/ directory..."
    rm -rf docs/man
fi

echo "✅ Documentation cleaned successfully!"
echo "Run './docs/scripts/generate_docs.sh' to regenerate documentation."
