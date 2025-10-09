#!/bin/bash

# Serve documentation locally
# This script starts a local HTTP server to view the generated documentation

set -e  # Exit on any error

echo "🌐 Starting documentation server..."

# Check if documentation exists
if [ ! -d "docs/html" ] || [ ! -f "docs/html/index.html" ]; then
    echo "❌ Documentation not found!"
    echo "Please generate documentation first:"
    echo "  make docs"
    echo "  or"
    echo "  ./docs/scripts/generate_docs.sh"
    exit 1
fi

# Check if port 8000 is available
if lsof -Pi :8000 -sTCP:LISTEN -t >/dev/null 2>&1; then
    echo "⚠️  Port 8000 is already in use. Trying port 8001..."
    PORT=8001
else
    PORT=8000
fi

echo "📖 Serving documentation on http://localhost:$PORT"
echo "📁 Documentation directory: docs/html/"
echo ""
echo "Press Ctrl+C to stop the server"
echo ""

# Start the server
cd docs/html && python3 -m http.server $PORT
