# unilink

![unilink logo](docs/assets/logo/light/unilink-logo-horizontal-readme-light.png#gh-light-mode-only)
![unilink logo](docs/assets/logo/dark/unilink-logo-horizontal-readme-dark.png#gh-dark-mode-only)

**Unified async communication for modern C++.**

Serial · TCP · UDP · UDS

![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows%20%7C%20macOS-informational)
![vcpkg](https://img.shields.io/badge/vcpkg-jwsung91--unilink-0078D6)
[![Coverage](https://img.shields.io/endpoint?url=https://jwsung91.github.io/unilink/coverage/badges/coverage.json)](https://jwsung91.github.io/unilink/coverage/)

## Description

Simple async C++ communication library for Serial, TCP, UDP, and Unix Domain Sockets

`unilink` provides a unified interface for asynchronous communication across different transports, allowing applications to switch between Serial, TCP, UDP, and UDS with minimal code changes. The public C++ API exposes builders and wrappers for all four transport families.

The project prioritizes **API clarity, predictable runtime behavior, and stability** over rapid feature expansion.

## Feature Highlights

* **Unified transport surface**: Consistent builders and wrappers for TCP client/server, UDP, Serial, and UDS.
* **Zero-copy memory safety via SafeSpan**: Efficient data handling without unnecessary copies.
* **Fluent API with CRTP Builders**: Type-safe configuration with improved method chaining.
* **Tested runtime behavior**: Unit, integration, and end-to-end test suites are part of the repository and documented in `test/`.

## Requirements

* **C++20 compiler and standard library**: GCC 10+, recent Clang/libc++, or MSVC 2022 (required)
* CMake 3.12 or later for plain builds; CMake 3.21 or later for the repository presets
* Boost 1.83.0 or later. vcpkg is the recommended dependency supplier; OS package manager Boost versions are supported only when they meet this minimum.

## 📦 Installation

### vcpkg (recommended)

```bash
vcpkg install jwsung91-unilink
```

For CMake usage, source builds, and other installation options, see the [Installation Guide](docs/user/installation.md).
Container images are maintained separately in [unilink-lab/unilink-containers](https://github.com/unilink-lab/unilink-containers).

### Contributor Development Setup

```bash
./scripts/setup_dev_env.sh
cmake --preset dev-linux-x64
cmake --build --preset dev-linux-x64
```

The setup script installs Boost and spdlog through an untracked, repository-local `vcpkg/` checkout by default. Delete that directory any time to reclaim space; rerun the setup script to recreate it. Set `VCPKG_ROOT` before running the script if you want to reuse an external vcpkg checkout.
CMake remains the version gate and rejects Boost versions older than 1.83.0.
The preset-based contributor workflow uses `CMakePresets.json` schema version 3, so those `cmake --preset ...` commands require CMake 3.21+.

## 📚 Documentation

Documentation is split by audience. Visit our **[Online Documentation](https://jwsung91.github.io/unilink/)** for the full experience.

### 📖 For Library Users

* [Quick Start Guide](docs/user/quickstart.md) – Get up and running in minutes
* [Installation Guide](docs/user/installation.md) – Package, source, and build options
* [Requirements](docs/user/requirements.md) – Supported platforms and dependencies
* [API Reference](docs/user/api_guide.md) – Public API overview
* [Python Bindings](https://github.com/unilink-lab/unilink-python) – External repository
* [Troubleshooting](docs/user/troubleshooting.md) – Common issues and resolutions
* [Tutorials](docs/user/tutorials/)

### 🔧 For Contributors

* [Build Guide](docs/contributor/build_guide.md) – Build configurations, CMake flags, sanitizers
* [Testing Guide](docs/contributor/testing.md) – Running tests and CI integration
* [Orin Nano Validation](docs/contributor/orin_nano_validation.md) – Ubuntu 22.04 ARM64 bring-up and test runbook
* [Implementation Status](docs/contributor/implementation_status.md) – Verified scope and known gaps
* [Architecture Overview](docs/contributor/architecture/README.md) – High-level structure and layer responsibilities
* [Runtime Behavior](docs/contributor/architecture/runtime_behavior.md) – Threading model, reconnection, backpressure
* [Memory Safety](docs/contributor/architecture/memory_safety.md) – Ownership rules and safety guarantees

### 💡 Examples

* [External examples repository](https://github.com/unilink-lab/unilink-examples)
* [Documentation Index](docs/index.md)

---

## 📄 License

**unilink** is released under the Apache License, Version 2.0.

Commercial use, modification, and redistribution are permitted.
For details, see the [LICENSE](./LICENSE) and [NOTICE](./NOTICE) files.

Copyright © 2025 Jinwoo Sung
