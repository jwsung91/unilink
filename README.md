# unilink

<p align="center">
  <img src="docs/img/logo.png" width="300">
</p>

![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows%20%7C%20macOS-informational)
![vcpkg](https://img.shields.io/badge/vcpkg-jwsung91--unilink-0078D6)
[![Coverage](https://img.shields.io/endpoint?url=https://jwsung91.github.io/unilink/coverage/badges/coverage.json)](https://github.com/jwsung91/unilink)

## Description

Simple async C++ communication library for Serial, TCP, UDP, and Unix Domain Sockets

`unilink` provides a unified interface for asynchronous communication across different transports, allowing applications to switch between Serial, TCP, UDP, and UDS with minimal code changes. The public C++ API exposes builders and wrappers for all four transport families; Python bindings cover Serial, TCP, UDP, and UDS.

The project prioritizes **API clarity, predictable runtime behavior, and stability** over rapid feature expansion.

## Feature Highlights

* **Unified transport surface**: Consistent builders and wrappers for TCP client/server, UDP, Serial, and UDS.
* **Zero-copy memory safety via SafeSpan**: Efficient data handling without unnecessary copies.
* **Fluent API with CRTP Builders**: Type-safe configuration with improved method chaining.
* **Tested runtime behavior**: Unit, integration, and end-to-end test suites are part of the repository and documented in `test/`.

## Requirements

* **C++17 compliant compiler**: GCC 11+ or Clang 14+ (required)
* CMake 3.12 or later

## 📦 Installation

### vcpkg (recommended)

```bash
vcpkg install jwsung91-unilink
```

For CMake usage, source builds, and other installation options, see the [Installation Guide](docs/user/installation.md).

## 🐍 Python Bindings

Unilink provides Python bindings for all core transport families (`TcpClient`, `TcpServer`, `Serial`, `Udp`, `UdsClient`, `UdsServer`). 

For a complete guide, see the **[Python Bindings Guide](docs/user/python_bindings.md)**.

### Prerequisites

* Python 3.8+ (dev headers)
* pybind11 (`sudo apt-get install pybind11-dev python3-pybind11` on Ubuntu)
* Boost (`boost-system`, `boost-asio`, `boost-thread`)

### Build

Pass `-DBUILD_PYTHON_BINDINGS=ON` to CMake:

```bash
cmake -B build -S . -DBUILD_PYTHON_BINDINGS=ON
cmake --build build
```

This generates `unilink_py.cpython-*.so` in `build/lib`. Add this directory to your `PYTHONPATH` to use it:

```bash
export PYTHONPATH=$PWD/build/lib
python3 -c "import unilink_py; print(unilink_py.__doc__)"
```

## 📚 Documentation

Documentation is split by audience. Visit our **[Online Documentation](https://jwsung91.github.io/unilink/)** for the full experience.

### 📖 For Library Users

* [Quick Start Guide](docs/user/quickstart.md) – Get up and running in minutes
* [Installation Guide](docs/user/installation.md) – Package, source, and build options
* [Requirements](docs/user/requirements.md) – Supported platforms and dependencies
* [API Reference](docs/user/api_guide.md) – Public API overview
* [Troubleshooting](docs/user/troubleshooting.md) – Common issues and resolutions
* [Tutorials](docs/user/tutorials/)

### 🔧 For Contributors

* [Build Guide](docs/contributor/build_guide.md) – Build configurations, CMake flags, sanitizers
* [Testing Guide](docs/contributor/testing.md) – Running tests and CI integration
* [Implementation Status](docs/contributor/implementation_status.md) – Verified scope and known gaps
* [Architecture Overview](docs/contributor/architecture/README.md) – High-level structure and layer responsibilities
* [Runtime Behavior](docs/contributor/architecture/runtime_behavior.md) – Threading model, reconnection, backpressure
* [Memory Safety](docs/contributor/architecture/memory_safety.md) – Ownership rules and safety guarantees

### 💡 Examples

* [TCP Examples](examples/tcp/)
* [UDP Examples](examples/udp/)
* [Serial Examples](examples/serial/)
* [UDS Examples](examples/uds/)
* [Common Examples](examples/common/)
* [Documentation Index](docs/index.md)

---

## 📄 License

**unilink** is released under the Apache License, Version 2.0.

Commercial use, modification, and redistribution are permitted.
For details, see the [LICENSE](./LICENSE) and [NOTICE](./NOTICE) files.

Copyright © 2025 Jinwoo Sung
