#!/bin/bash
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

echo "🧹 Cleaning workspace..."
rm -rf build/
rm -rf Testing/
find . -name "*.log" -type f -delete
find . -name ".DS_Store" -type f -delete

echo "✨ Workspace is clean."
