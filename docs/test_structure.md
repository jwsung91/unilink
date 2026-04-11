# Test Structure

This document describes how the test tree is organized and how to work with it, without embedding release-specific counts or dated snapshots.

## Layout

```text
test/
├── unit/          # Unit and transport-focused tests
├── integration/   # Cross-component and real I/O integration tests
├── e2e/           # End-to-end scenarios and stress tests
├── performance/   # Optional benchmarks and profiling tests
├── fixtures/      # Shared mocks and fixtures
├── mocks/         # Additional mock types used by tests
├── utils/         # Shared test helpers and constants
└── CMakeLists.txt # Main test registration
```

## What Each Area Covers

- `unit/`: isolated component behavior, transport edge cases, wrapper behavior, framing, config, and utility coverage
- `integration/`: interaction between components and real I/O paths
- `e2e/`: scenario-level behavior, recovery cases, and stress-oriented coverage
- `performance/`: optional benchmarks that are only registered when explicitly enabled

## Build-Time Controls

The test tree is controlled by CMake options rather than hardcoded assumptions in this document.

- `UNILINK_BUILD_TESTS`: enables test targets
- `UNILINK_ENABLE_PERFORMANCE_TESTS`: registers optional performance tests

Treat `test/CMakeLists.txt` and the active build directory as the source of truth for what is currently registered.

## Running Tests

### Run All Registered Tests

```bash
cd build
ctest --output-on-failure
```

### Run By Broad Category

```bash
ctest -L unit
ctest -L integration
ctest -L e2e
ctest -L performance
```

`performance` only returns results if the build was configured with `-DUNILINK_ENABLE_PERFORMANCE_TESTS=ON`.

### Useful Focused Runs

```bash
# TCP-heavy tests
ctest -L tcp

# Builder-related tests
ctest -L builder

# Security and contract checks
ctest -L security
ctest -L contract
```

### Inspect What Is Currently Registered

```bash
ctest -N
ctest -N -L unit
ctest -N -L integration
ctest -N -L e2e
```

Use these commands instead of storing counts in documentation. The exact number of tests changes as coverage grows.

## Notes

- Label sets are not necessarily exclusive.
- Performance sources may exist in the tree even when no performance tests are currently registered.
- When test organization changes, update the commands and directory descriptions here, not the output of a particular local run.

## CI/CD Integration

Repository workflows live under `.github/workflows/`. If labels or test grouping changes, keep CI filters in sync with the commands documented here.
