# Test Structure

This document describes the organized test structure of the `unilink` library.

---

## Overview

Tests are organized into **4 main categories**:

| Category | Purpose | Speed | Test Count | Labels |
|----------|---------|-------|------------|--------|
| **Unit** | Isolated component tests | Fast (~10s) | 97 | `unit`, `fast` |
| **Integration** | Component interaction tests | Medium (~75s) | 118 | `integration`, `medium` |
| **E2E** | End-to-end scenarios | Slow (~64s) | 22 | `e2e`, `slow` |
| **Performance** | Benchmarks and profiling | Optional (~10s) | 58 | `performance`, `optional` |

**Total**: 295 tests (~160 seconds full suite)

---

## Directory Structure

```
test/
├── unit/                       # Fast unit tests
│   ├── common/                 # Common utilities tests
│   │   ├── test_core.cc
│   │   ├── test_memory.cc
│   │   ├── test_boundary.cc
│   │   └── test_error_handler.cc
│   ├── builder/                # Builder pattern tests
│   │   ├── test_builder.cc
│   │   └── test_builder_coverage.cc
│   ├── config/                 # Configuration tests
│   │   └── test_config.cc
│   └── CMakeLists.txt
│
├── integration/                # Integration tests
│   ├── tcp/                    # TCP communication tests
│   │   ├── test_integration.cc
│   │   ├── test_simple_server.cc
│   │   └── test_client_limit_integration.cc
│   ├── serial/                 # Serial communication tests
│   │   ├── test_serial.cc
│   │   └── test_serial_builder_improvements.cc
│   ├── mock/                   # Mock-based tests
│   │   ├── test_mock_integration.cc
│   │   └── test_mock_integrated.cc
│   ├── test_builder_integration.cc
│   ├── test_stable_integration.cc
│   ├── test_core_integrated.cc
│   ├── test_safety_integrated.cc
│   └── CMakeLists.txt
│
├── e2e/                        # End-to-end tests
│   ├── scenarios/              # Real-world scenarios
│   │   ├── test_architecture.cc
│   │   └── test_error_recovery.cc
│   ├── stress/                 # Stress tests
│   │   └── test_stress.cc
│   └── CMakeLists.txt
│
├── performance/                # Performance tests
│   ├── benchmark/              # Benchmarks
│   │   ├── test_performance.cc
│   │   ├── test_benchmark.cc
│   │   ├── test_transport_performance.cc
│   │   └── test_platform.cc
│   └── CMakeLists.txt
│
├── fixtures/                   # Shared test resources
│   ├── mocks/                  # Mock objects
│   └── data/                   # Test data
│
├── utils/                      # Test utilities
│   └── test_utils.hpp
│
└── CMakeLists.txt              # Main test configuration
```

---

## Running Tests

### Run All Tests

```bash
cd build
ctest --output-on-failure
```

### Run by Category

```bash
# Fast feedback (unit tests only)
ctest -L unit

# Medium tests (integration)
ctest -L integration

# Slow tests (E2E)
ctest -L e2e

# Performance tests
ctest -L performance
```

### Run by Component

```bash
# All TCP-related tests
ctest -L tcp

# All builder-related tests
ctest -L builder

# All mock tests
ctest -L mock
```

### Run Specific Test Executable

```bash
# Run specific test file
./test/unit/run_unit_test_builder --gtest_filter="BuilderTest.*"

# List all tests
./test/unit/run_unit_test_builder --gtest_list_tests

# Run with verbose output
./test/unit/run_unit_test_builder --gtest_verbose
```

---

## CI/CD Integration

### GitHub Actions Workflows

The project uses a **staged testing approach** in CI:

#### 1. **Pull Request** (Fast feedback)
- ✅ Unit Tests (~10s)
- ✅ Integration Tests (~75s)
- ⏭️ E2E Tests (skipped for PRs)

#### 2. **Main/Develop Branch** (Full validation)
- ✅ Unit Tests
- ✅ Integration Tests
- ✅ E2E Tests (~64s)

#### 3. **Nightly/On-Demand** (Performance)
- ✅ Performance Tests (~10s)
- 📊 Benchmark results archived

### Workflow Files

- `.github/workflows/test-organized.yml` - Organized test execution
- `.github/workflows/ci.yml` - Full CI/CD pipeline
- `.github/workflows/coverage.yml` - Code coverage analysis

---

## Test Results Summary

Latest test run results:

```
Unit Tests:         91% pass rate (88/97)   ~10s
Integration Tests:  83% pass rate (98/118)  ~75s
E2E Tests:          91% pass rate (20/22)   ~64s
Performance Tests:  100% pass rate (58/58)  ~10s
----------------------------------------
Total:              89% pass rate (264/295) ~160s
```

---

## Benefits of Organized Structure

### 1. **Faster Feedback** ⚡
- PR feedback in <2 minutes (unit tests only)
- Parallel test execution
- Selective test running

### 2. **Better Maintainability** 🔧
- Clear test organization
- Easy to find specific tests
- Logical grouping by functionality

### 3. **Improved Developer Experience** 👨‍💻
```bash
# Quick check during development
ctest -L unit

# Full validation before commit
ctest -L "unit|integration"

# Performance regression check
ctest -L performance
```

### 4. **CI Optimization** 🚀
- Unit tests on every PR
- Integration tests on develop/main
- E2E tests on main only
- Performance tests nightly

---

## Adding New Tests

### 1. Choose the Right Category

- **Unit Test**: Testing a single class/function in isolation
- **Integration Test**: Testing interaction between components
- **E2E Test**: Testing complete user scenarios
- **Performance Test**: Benchmarking and profiling

### 2. Add Test File

```cpp
// test/unit/common/test_my_feature.cc
#include <gtest/gtest.h>
#include "unilink/my_feature.hpp"

TEST(MyFeatureTest, BasicFunctionality) {
  // Your test here
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
```

### 3. Update CMakeLists.txt

```cmake
# test/unit/CMakeLists.txt
add_executable(run_unit_test_my_feature common/test_my_feature.cc)
target_link_libraries(run_unit_test_my_feature
  PRIVATE
    unilink
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)
gtest_discover_tests(run_unit_test_my_feature
  PROPERTIES
    LABELS "unit;common;fast"
    TIMEOUT 30
)
```

### 4. Build and Run

```bash
cd build
cmake --build . --target run_unit_test_my_feature
./test/unit/run_unit_test_my_feature
```

---

## Test Best Practices

### Unit Tests
- ✅ Fast (< 1 second per test)
- ✅ Isolated (no external dependencies)
- ✅ Deterministic (same result every time)
- ✅ Focus on single responsibility

### Integration Tests
- ✅ Test component interactions
- ✅ Use mocks when appropriate
- ✅ Keep under 60 seconds
- ✅ Test realistic scenarios

### E2E Tests
- ✅ Test complete user workflows
- ✅ Minimize test count (high value only)
- ✅ Can be slower (up to 2 minutes)
- ✅ Test critical paths

### Performance Tests
- ✅ Measure performance metrics
- ✅ Detect regressions
- ✅ Archive results for comparison
- ✅ Run in Release mode

---

## Troubleshooting

### Test Failures

```bash
# Run specific test with verbose output
ctest -R "test_name" --verbose --output-on-failure

# Run test directly (more control)
./test/unit/run_unit_test_builder --gtest_filter="BuilderTest.BasicFunctionality"

# Run with debugging
gdb ./test/unit/run_unit_test_builder
```

### Build Issues

```bash
# Clean rebuild
rm -rf build
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build

# Check test targets
cmake --build build --target help | grep test
```

### Timeout Issues

If tests timeout in CI:
1. Check test timeout settings in CMakeLists.txt
2. Reduce test complexity
3. Use mocks instead of real I/O
4. Consider moving to E2E category

---

## Future Improvements

- [ ] Add code coverage per test category
- [ ] Implement test sharding for parallel execution
- [ ] Add test result trending and analytics
- [ ] Create test templates for common patterns
- [ ] Add mutation testing for test quality
- [ ] Implement visual test reports

---

## References

- [Google Test Documentation](https://google.github.io/googletest/)
- [CTest Documentation](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
- [Test Coverage Guide](./COVERAGE_IMPROVEMENT_GUIDE.md)
- [Test Refactoring Plan](./TEST_REFACTORING_PLAN.md)

