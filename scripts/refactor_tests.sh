#!/bin/bash

# Test Refactoring Script
# Moves test files to new organized structure

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
TEST_DIR="$PROJECT_ROOT/test"

echo "=== Test Refactoring Script ==="
echo "Project Root: $PROJECT_ROOT"
echo "Test Directory: $TEST_DIR"
echo ""

# ============================================================================
# UNIT TESTS - Fast, isolated tests
# ============================================================================

echo "📦 Moving Unit Tests..."

# Common utility tests
mv "$TEST_DIR/test_common.cc" "$TEST_DIR/unit/common/" 2>/dev/null || echo "  - test_common.cc already moved or not found"
mv "$TEST_DIR/test_core.cc" "$TEST_DIR/unit/common/" 2>/dev/null || echo "  - test_core.cc already moved or not found"
mv "$TEST_DIR/test_memory.cc" "$TEST_DIR/unit/common/" 2>/dev/null || echo "  - test_memory.cc already moved or not found"
mv "$TEST_DIR/test_boundary.cc" "$TEST_DIR/unit/common/" 2>/dev/null || echo "  - test_boundary.cc already moved or not found"
mv "$TEST_DIR/test_error_handler_coverage_fixed.cc" "$TEST_DIR/unit/common/test_error_handler.cc" 2>/dev/null || echo "  - test_error_handler.cc already moved or not found"

# Builder tests
mv "$TEST_DIR/test_builder.cc" "$TEST_DIR/unit/builder/" 2>/dev/null || echo "  - test_builder.cc already moved or not found"
mv "$TEST_DIR/test_builder_coverage.cc" "$TEST_DIR/unit/builder/" 2>/dev/null || echo "  - test_builder_coverage.cc already moved or not found"

# Config tests
mv "$TEST_DIR/test_config.cc" "$TEST_DIR/unit/config/" 2>/dev/null || echo "  - test_config.cc already moved or not found"

# ============================================================================
# INTEGRATION TESTS - Component integration
# ============================================================================

echo "🔗 Moving Integration Tests..."

# TCP integration
mv "$TEST_DIR/test_integration.cc" "$TEST_DIR/integration/tcp/" 2>/dev/null || echo "  - test_integration.cc already moved or not found"
mv "$TEST_DIR/test_communication.cc" "$TEST_DIR/integration/tcp/" 2>/dev/null || echo "  - test_communication.cc already moved or not found"
mv "$TEST_DIR/test_simple_server.cc" "$TEST_DIR/integration/tcp/" 2>/dev/null || echo "  - test_simple_server.cc already moved or not found"
mv "$TEST_DIR/test_client_limit_integration.cc" "$TEST_DIR/integration/tcp/" 2>/dev/null || echo "  - test_client_limit_integration.cc already moved or not found"

# Serial integration
mv "$TEST_DIR/test_serial.cc" "$TEST_DIR/integration/serial/" 2>/dev/null || echo "  - test_serial.cc already moved or not found"
mv "$TEST_DIR/test_serial_builder_improvements.cc" "$TEST_DIR/integration/serial/" 2>/dev/null || echo "  - test_serial_builder_improvements.cc already moved or not found"

# Mock integration
mv "$TEST_DIR/test_mock_integration.cc" "$TEST_DIR/integration/mock/" 2>/dev/null || echo "  - test_mock_integration.cc already moved or not found"
mv "$TEST_DIR/test_mock_integrated.cc" "$TEST_DIR/integration/mock/" 2>/dev/null || echo "  - test_mock_integrated.cc already moved or not found"

# General integration
mv "$TEST_DIR/test_builder_integration.cc" "$TEST_DIR/integration/" 2>/dev/null || echo "  - test_builder_integration.cc already moved or not found"
mv "$TEST_DIR/test_stable_integration.cc" "$TEST_DIR/integration/" 2>/dev/null || echo "  - test_stable_integration.cc already moved or not found"
mv "$TEST_DIR/test_core_integrated.cc" "$TEST_DIR/integration/" 2>/dev/null || echo "  - test_core_integrated.cc already moved or not found"
mv "$TEST_DIR/test_safety_integrated.cc" "$TEST_DIR/integration/" 2>/dev/null || echo "  - test_safety_integrated.cc already moved or not found"

# ============================================================================
# E2E TESTS - End-to-end scenarios
# ============================================================================

echo "🎯 Moving E2E Tests..."

# Stress tests
mv "$TEST_DIR/test_stress.cc" "$TEST_DIR/e2e/stress/" 2>/dev/null || echo "  - test_stress.cc already moved or not found"

# Scenarios
mv "$TEST_DIR/test_error_recovery.cc" "$TEST_DIR/e2e/scenarios/" 2>/dev/null || echo "  - test_error_recovery.cc already moved or not found"
mv "$TEST_DIR/test_architecture.cc" "$TEST_DIR/e2e/scenarios/" 2>/dev/null || echo "  - test_architecture.cc already moved or not found"

# ============================================================================
# PERFORMANCE TESTS - Benchmarks and profiling
# ============================================================================

echo "⚡ Moving Performance Tests..."

# Benchmarks
mv "$TEST_DIR/test_performance.cc" "$TEST_DIR/performance/benchmark/" 2>/dev/null || echo "  - test_performance.cc already moved or not found"
mv "$TEST_DIR/test_benchmark.cc" "$TEST_DIR/performance/benchmark/" 2>/dev/null || echo "  - test_benchmark.cc already moved or not found"
mv "$TEST_DIR/test_transport_performance.cc" "$TEST_DIR/performance/benchmark/" 2>/dev/null || echo "  - test_transport_performance.cc already moved or not found"
mv "$TEST_DIR/test_platform.cc" "$TEST_DIR/performance/benchmark/" 2>/dev/null || echo "  - test_platform.cc already moved or not found"

# Profiling
mv "$TEST_DIR/test_advanced_optimizations.cc" "$TEST_DIR/performance/profiling/" 2>/dev/null || echo "  - test_advanced_optimizations.cc already moved or not found"

# ============================================================================
# UTILITIES
# ============================================================================

echo "🛠️  Setting up utilities..."

# Already copied earlier, now create symlinks in utils if needed

echo ""
echo "✅ Test refactoring complete!"
echo ""
echo "Next steps:"
echo "1. Update CMakeLists.txt files"
echo "2. Build and test"
echo "3. Verify all tests pass"

