# Test Refactoring Plan

## Current State

- **Files:** 28 test files
- **Test Cases:** 245+ tests
- **Execution Time:** ~55 seconds (full suite)
- **Structure:** Flat single directory

## Issues

1. ❌ Difficult to find specific tests
2. ❌ Cannot run test subsets efficiently
3. ❌ Hard to maintain (too many files in one directory)
4. ❌ CI runs all tests even for small changes
5. ❌ Some tests take too long (integration/e2e)

---

## Proposed Structure

```
test/
├── unit/                          # Fast, isolated tests
│   ├── common/
│   │   ├── test_logger.cc
│   │   ├── test_memory_pool.cc
│   │   ├── test_safe_data_buffer.cc
│   │   ├── test_thread_safe_state.cc
│   │   └── test_error_handler.cc
│   ├── builder/
│   │   ├── test_tcp_server_builder.cc
│   │   ├── test_tcp_client_builder.cc
│   │   ├── test_serial_builder.cc
│   │   └── test_unified_builder.cc
│   ├── config/
│   │   ├── test_config_manager.cc
│   │   └── test_config_validator.cc
│   ├── validator/
│   │   └── test_input_validator.cc
│   └── CMakeLists.txt
│
├── integration/                   # Component integration tests
│   ├── tcp/
│   │   ├── test_tcp_client_server.cc
│   │   ├── test_tcp_multi_client.cc
│   │   └── test_tcp_reconnection.cc
│   ├── serial/
│   │   ├── test_serial_communication.cc
│   │   └── test_serial_retry.cc
│   ├── mock/
│   │   ├── test_mock_integration.cc
│   │   └── test_dependency_injection.cc
│   └── CMakeLists.txt
│
├── e2e/                          # End-to-end scenarios
│   ├── scenarios/
│   │   ├── test_echo_server.cc
│   │   ├── test_chat_application.cc
│   │   └── test_data_pipeline.cc
│   ├── stress/
│   │   ├── test_high_load.cc
│   │   ├── test_memory_stress.cc
│   │   └── test_connection_storm.cc
│   └── CMakeLists.txt
│
├── performance/                   # Performance benchmarks
│   ├── benchmark/
│   │   ├── test_throughput.cc
│   │   ├── test_latency.cc
│   │   └── test_memory_pool_perf.cc
│   ├── profiling/
│   │   └── test_cpu_profiling.cc
│   └── CMakeLists.txt
│
├── fixtures/                      # Test utilities
│   ├── mocks/
│   │   ├── mock_tcp_socket.hpp
│   │   ├── mock_serial_port.hpp
│   │   └── mock_test_helpers.hpp
│   └── data/
│       ├── test_configs/
│       └── sample_data/
│
└── utils/                         # Shared test utilities
    ├── test_base.hpp
    ├── test_helpers.hpp
    └── CMakeLists.txt
```

---

## File Migration Map

### Unit Tests (15 files → unit/)

| Current File | New Location | Reason |
|-------------|--------------|--------|
| `test_common.cc` | `unit/common/test_common.cc` | Common utilities test |
| `test_core.cc` | `unit/common/test_core.cc` | Core functionality |
| `test_memory.cc` | `unit/common/test_memory.cc` | Memory management |
| `test_builder.cc` | `unit/builder/test_builder_base.cc` | Base builder tests |
| `test_builder_coverage.cc` | `unit/builder/test_builder_coverage.cc` | Builder coverage |
| `test_config.cc` | `unit/config/test_config_manager.cc` | Config manager |
| `test_error_handler_coverage_fixed.cc` | `unit/common/test_error_handler.cc` | Error handling |
| `test_boundary.cc` | `unit/common/test_boundary.cc` | Boundary conditions |
| `test_core_integrated.cc` | Split → unit/ | Core unit tests |
| `test_safety_integrated.cc` | Split → unit/ | Safety unit tests |

### Integration Tests (10 files → integration/)

| Current File | New Location | Reason |
|-------------|--------------|--------|
| `test_integration.cc` | `integration/tcp/test_tcp_integration.cc` | TCP integration |
| `test_builder_integration.cc` | `integration/test_builder_integration.cc` | Builder integration |
| `test_stable_integration.cc` | `integration/test_stable_integration.cc` | Stable integration |
| `test_communication.cc` | `integration/tcp/test_communication.cc` | Communication tests |
| `test_simple_server.cc` | `integration/tcp/test_simple_server.cc` | Simple server |
| `test_client_limit_integration.cc` | `integration/tcp/test_client_limit.cc` | Client limits |
| `test_mock_integration.cc` | `integration/mock/test_mock_integration.cc` | Mock tests |
| `test_mock_integrated.cc` | `integration/mock/test_mock_integrated.cc` | Integrated mocks |
| `test_serial.cc` | `integration/serial/test_serial_integration.cc` | Serial tests |
| `test_serial_builder_improvements.cc` | `integration/serial/test_serial_builder.cc` | Serial builder |

### E2E Tests (3 files → e2e/)

| Current File | New Location | Reason |
|-------------|--------------|--------|
| `test_stress.cc` | `e2e/stress/test_stress.cc` | Stress testing |
| `test_error_recovery.cc` | `e2e/scenarios/test_error_recovery.cc` | Error recovery |
| `test_architecture.cc` | `e2e/scenarios/test_architecture.cc` | Architecture tests |

### Performance Tests (5 files → performance/)

| Current File | New Location | Reason |
|-------------|--------------|--------|
| `test_performance.cc` | `performance/benchmark/test_performance.cc` | Performance bench |
| `test_benchmark.cc` | `performance/benchmark/test_benchmark.cc` | Benchmarks |
| `test_transport_performance.cc` | `performance/benchmark/test_transport.cc` | Transport perf |
| `test_advanced_optimizations.cc` | `performance/profiling/test_optimizations.cc` | Optimizations |
| `test_platform.cc` | `performance/benchmark/test_platform.cc` | Platform tests |

---

## Benefits

### 1. **Faster CI/CD** ⚡

```yaml
# Run only relevant tests
- Unit tests: ~10 seconds
- Integration tests: ~20 seconds  
- E2E tests: ~25 seconds
- Performance tests: ~30 seconds (optional)

# Smart test selection
- PR to builder/ → run unit/builder/ + integration/
- PR to transport/ → run unit/ + integration/tcp/
```

### 2. **Better Developer Experience** 👨‍💻

```bash
# Run specific test groups
ctest -L unit              # Fast feedback (10s)
ctest -L integration       # Medium tests (20s)
ctest -L e2e              # Full scenarios (25s)
ctest -L performance      # Benchmarks (30s)

# Or by component
ctest -L tcp              # All TCP tests
ctest -L builder          # All builder tests
```

### 3. **Improved Maintainability** 🔧

- Easy to find specific tests
- Clear test responsibilities
- Better test organization
- Reduced cognitive load

### 4. **Coverage Optimization** 📊

- Identify coverage gaps per component
- Focus on high-value tests
- Remove redundant tests
- Better test/coverage ratio

---

## Implementation Steps

### Phase 1: Setup New Structure (1 hour)

```bash
# 1. Create new directory structure
mkdir -p test/{unit,integration,e2e,performance}/{common,builder,config,validator,tcp,serial,mock,scenarios,stress,benchmark,profiling}
mkdir -p test/{fixtures/{mocks,data},utils}

# 2. Move utility files
mv test/test_utils.hpp test/utils/
mv test/mocks/* test/fixtures/mocks/

# 3. Create stub CMakeLists.txt files
for dir in unit integration e2e performance utils; do
  touch test/$dir/CMakeLists.txt
done
```

### Phase 2: Migrate Tests (2-3 hours)

Priority order:
1. ✅ Unit tests (most files, easiest to move)
2. ✅ Integration tests (some refactoring needed)
3. ✅ Performance tests (may need splitting)
4. ✅ E2E tests (may need consolidation)

### Phase 3: Update CMake Configuration (1 hour)

```cmake
# test/CMakeLists.txt (root)
add_subdirectory(utils)
add_subdirectory(unit)
add_subdirectory(integration)
add_subdirectory(e2e)

# Optional for CI
option(BUILD_PERFORMANCE_TESTS "Build performance tests" ON)
if(BUILD_PERFORMANCE_TESTS)
  add_subdirectory(performance)
endif()

# Test labels for selective execution
set_tests_properties(UnitTests PROPERTIES LABELS "unit;fast")
set_tests_properties(IntegrationTests PROPERTIES LABELS "integration;medium")
set_tests_properties(E2ETests PROPERTIES LABELS "e2e;slow")
set_tests_properties(PerformanceTests PROPERTIES LABELS "performance;benchmark")
```

### Phase 4: Update CI/CD (1 hour)

```yaml
# .github/workflows/test-fast.yml (for PRs)
- name: Run Unit Tests
  run: ctest -L unit --output-on-failure

- name: Run Integration Tests
  run: ctest -L integration --output-on-failure

# .github/workflows/test-full.yml (for main/develop)
- name: Run All Tests
  run: ctest --output-on-failure

# .github/workflows/test-performance.yml (nightly)
- name: Run Performance Tests
  run: ctest -L performance --output-on-failure
```

---

## Test Optimization Strategies

### 1. **Remove Redundant Tests**

Identify and remove duplicate or overlapping tests:

```bash
# Find similar test names
find test -name "test_*.cc" | sort | uniq -d

# Example duplicates to review:
- test_integration.cc vs test_builder_integration.cc
- test_mock_integration.cc vs test_mock_integrated.cc
- test_core.cc vs test_core_integrated.cc
```

**Action:** Merge similar tests, keep unique functionality

### 2. **Speed Up Slow Tests**

```cpp
// Before: Wait for real timeout
std::this_thread::sleep_for(std::chrono::seconds(5));

// After: Use minimal waits
TestUtils::waitFor(100);  // 100ms

// Before: Create real connections
auto server = create_real_server();

// After: Use mocks when possible
auto server = create_mock_server();
```

### 3. **Parallelize Tests**

```cmake
# Enable parallel test execution
set_tests_properties(test_name PROPERTIES
  RUN_SERIAL FALSE
  TIMEOUT 30
)

# Avoid port conflicts
- Use dynamic port allocation
- Use test fixtures with unique resources
```

### 4. **Test Data Caching**

```cpp
// Cache expensive setup
class PerformanceTestFixture : public ::testing::Test {
  static void SetUpTestSuite() {
    // Run once for all tests in suite
    global_resource = setup_expensive_resource();
  }
  
  static void TearDownTestSuite() {
    cleanup_resource(global_resource);
  }
};
```

---

## Metrics to Track

| Metric | Before | Target After |
|--------|--------|--------------|
| **Total Test Time** | ~55s | ~40s |
| **Unit Test Time** | N/A | <10s |
| **Integration Test Time** | N/A | <20s |
| **E2E Test Time** | N/A | <15s |
| **CI Feedback Time** | 5-8min | 2-3min |
| **Coverage** | 47.3% | 65%+ |
| **Test Files** | 28 | 25-30 |
| **Test Cases** | 245 | 250+ |

---

## Success Criteria

- [ ] All tests organized into logical groups
- [ ] CMake labels configured for selective testing
- [ ] CI updated to run fast tests on PR
- [ ] Full test suite runs nightly
- [ ] Test execution time reduced by 30%+
- [ ] Coverage maintained or improved
- [ ] Documentation updated
- [ ] Team onboarded to new structure

---

## Timeline

| Phase | Duration | Owner |
|-------|----------|-------|
| Planning & Review | 2 hours | Team |
| Setup Structure | 1 hour | Dev |
| Migrate Unit Tests | 2 hours | Dev |
| Migrate Integration Tests | 2 hours | Dev |
| Migrate E2E/Perf Tests | 1 hour | Dev |
| Update CMake | 1 hour | Dev |
| Update CI/CD | 1 hour | Dev |
| Testing & Validation | 2 hours | Team |
| **Total** | **12 hours** | **~2 days** |

---

## Next Steps

1. **Review this plan** with the team
2. **Create a branch** for refactoring (`refactor/test-structure`)
3. **Start with Phase 1** (setup structure)
4. **Migrate incrementally** (one category at a time)
5. **Test after each phase** (ensure nothing breaks)
6. **Update docs** as you go
7. **Submit PR** when complete

---

## Questions to Consider

1. Should we keep backward compatibility during migration?
2. Do we need a deprecation period for old test paths?
3. Should performance tests be optional in CI?
4. Do we want to add new test categories (e.g., `security/`)?
5. Should we standardize test naming conventions?

---

## Resources

- [Google Test Documentation](https://google.github.io/googletest/)
- [CTest Labels](https://cmake.org/cmake/help/latest/command/set_tests_properties.html)
- [CI Optimization Best Practices](https://docs.github.com/en/actions/using-workflows/about-workflows)

