# Test Structure {#contrib_test_structure}

This document describes how the test tree is organized and how to work with it, without embedding release-specific counts or dated snapshots.

## Layout

```text
test/
├── unit/          # Unit and transport-focused tests
├── integration/   # Cross-component and real I/O integration tests
├── e2e/           # End-to-end scenarios and stress tests
├── mocks/         # Additional mock types used by tests
├── utils/         # Shared test helpers and constants
└── CMakeLists.txt # Main test registration
```

## What Each Area Covers

- `unit/`: isolated component behavior, transport edge cases, wrapper behavior, framing, config, and utility coverage
- `integration/`: interaction between components and real I/O paths
- `e2e/`: scenario-level behavior, recovery cases, and stress-oriented coverage

Prefer adding new tests to the smallest scope that matches their behavior. If a
test opens localhost sockets, binds ports, creates UDS socket paths, or relies
on real OS I/O, place it in `integration/` unless it is intentionally tied to a
narrow unit fixture.

## Build-Time Controls

The test tree is controlled by CMake options rather than hardcoded assumptions in this document.

- `UNILINK_BUILD_TESTS`: enables test targets

Treat `test/CMakeLists.txt` and the active build directory as the source of truth for what is currently registered.

## Running Tests

### Run All Registered Tests

```bash
cd build
ctest --output-on-failure
```

### Run By Broad Category

```bash
# Run all unit tests (matches unit_* labels)
ctest -L "unit_.*"
# Run legacy integration tests
ctest -L "legacy_integration"
# Run all e2e tests (matches e2e_* labels)
ctest -L "e2e_.*"
```

Standalone benchmarks are maintained separately:
[unilink-lab/unilink-benchmarks](https://github.com/unilink-lab/unilink-benchmarks).

### Useful Focused Runs

```bash
# TCP-heavy tests
ctest -L tcp

# Builder-related tests
ctest -L builder

# Security and contract checks
ctest -L security
ctest -L contract

# Structured scope/component filters
ctest -L "integration.*transport.*tcp"
ctest -L "integration.*serial"
```

### Inspect What Is Currently Registered

```bash
ctest -N
ctest -N -L "unit_.*"
ctest -N -L "legacy_integration"
ctest -N -L "e2e_.*"
```

Use these commands instead of storing counts in documentation. The exact number of tests changes as coverage grows.

## Notes

- Labels use structured tokens such as `integration_transport_tcp_medium` so broad
  filters (`unit`, `tcp`) and composed regex filters
  (`integration.*transport.*tcp`) both work.
- Label names should follow
  `<scope>_<component>[_subcomponent]_<kind>[_io]`. Examples include
  `unit_common_fast`, `unit_transport_uds_fast`,
  `integration_transport_tcp_medium`, `integration_transport_uds_medium`,
  `integration_wrapper_udp_medium`, `integration_tcp_medium`,
  `e2e_scenario_slow`, and `docs_snippets`.
- Tests that allocate ports, open localhost sockets, or create UDS socket paths
  belong under `integration/`, even if they exercise a focused transport or
  wrapper behavior.
- When test organization changes, update the commands and directory descriptions here, not the output of a particular local run.

## CI/CD Integration

Repository workflows live under `.github/workflows/`. If labels or test grouping changes, keep CI filters in sync with the commands documented here.
