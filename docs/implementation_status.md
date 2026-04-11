# Implementation Status

This page summarizes the implementation scope of the repository without duplicating release-specific values that already live elsewhere.

## Scope

The repository currently contains implemented source trees for:

- C++ wrapper API
- Builder API
- Transport implementations
- Diagnostics and logging
- Configuration management
- Memory and framing utilities
- Examples and test suites
- Optional Python bindings

## C++ API Surface

The public umbrella header `unilink/unilink.hpp` exposes wrappers and builders for:

- TCP client
- TCP server
- UDP
- Serial
- UDS client
- UDS server

If you need the exact public entry points, treat `unilink/unilink.hpp` and the headers under `unilink/wrapper/` and `unilink/builder/` as the source of truth.

## Python Binding Scope

The Python module is implemented in `bindings/python/module.cpp`.

At a high level, `unilink_py` covers:

- TCP client
- TCP server
- UDP
- Serial
- UDS client
- UDS server

If binding coverage changes, update this section only when the exposed API surface changes, not when the package version changes.

## Build And Test Status

Dynamic build defaults and flags should be read from:

- `CMakeLists.txt`
- `cmake/UnilinkOptions.cmake`
- [Build Guide](guides/setup/build_guide.md)

Dynamic test registration and current pass/fail state should be read from:

- `test/CMakeLists.txt`
- [Test Structure](test_structure.md)
- `ctest` output from the active build directory

This document intentionally does not repeat exact version numbers, build-cache values, or test counts, because those change more often than the implementation scope itself.

## Recommended Reading Order

If you are trying to understand "what is implemented right now", read in this order:

1. `unilink/unilink.hpp`
2. [API Guide](reference/api_guide.md)
3. [Examples Directory](../examples/README.md)
4. [Test Structure](test_structure.md)
5. [Architecture Overview](architecture/README.md)
