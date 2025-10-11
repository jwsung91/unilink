#!/bin/bash

# Fix tcp_server builders to include unlimited_clients()
# This script adds .unlimited_clients() to tcp_server() calls that are missing it

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_DIR="$PROJECT_ROOT/test"

echo "=== Fixing TCP Server Builder Tests ==="
echo ""

# Find all test files with tcp_server
TEST_FILES=$(find "$TEST_DIR" -name "*.cc" -type f)

for file in $TEST_FILES; do
  # Check if file contains tcp_server
  if grep -q "tcp_server" "$file"; then
    echo "Processing: $file"
    
    # Pattern 1: tcp_server(port).build() → tcp_server(port).unlimited_clients().build()
    # But only if it doesn't already have unlimited_clients, single_client, or multi_client
    
    # Use perl for more complex regex
    perl -i -pe 's/tcp_server\(([^)]+)\)\.(?!.*(?:unlimited_clients|single_client|multi_client))(.+?)\.build\(\)/tcp_server($1).unlimited_clients().$2.build()/g' "$file"
    
    # Pattern 2: Handle cases where build() is on the next line
    # This is more complex and might need manual review
    
    echo "  ✓ Updated"
  fi
done

echo ""
echo "✅ Fix complete!"
echo "Please review the changes and run tests."

