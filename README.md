# unilink

<p align="center">
  <img src="docs/img/logo.png" width="300">
</p>

[![CI](https://github.com/jwsung91/unilink/actions/workflows/ci.yml/badge.svg)](https://github.com/jwsung91/unilink/actions/workflows/ci.yml)
[![Coverage](https://img.shields.io/endpoint?url=https://jwsung91.github.io/unilink/coverage/badges/coverage.json)](https://github.com/jwsung91/unilink)

![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows%20%7C%20macOS-informational)
![vcpkg](https://img.shields.io/badge/vcpkg-jwsung91--unilink-0078D6)

## Description

Simple, cross-platform async C++ communication library for Serial, TCP, and UDP

`unilink` provides a unified interface for asynchronous communication across different transports, allowing applications to switch between Serial, TCP, and UDP with minimal code changes.

The project prioritizes **API clarity, predictable runtime behavior, and stability** over rapid feature expansion.

## 📦 Installation

### vcpkg (recommended)

```bash
vcpkg install jwsung91-unilink
```

For CMake usage, source builds, and other installation options, see the [Installation Guide](docs/guides/setup/installation.md).

## 📚 Documentation

The documentation is organized by learning stage, from quick start to advanced topics.
You do **not** need to read everything to get started.

### 🚦 Getting Started

* [Quick Start Guide](docs/guides/core/quickstart.md) – Get up and running in minutes
* [Installation Guide](docs/guides/setup/installation.md) – Package, source, and build options
* [Build Guide](docs/guides/setup/build_guide.md) – Build configurations and flags
* [Requirements](docs/guides/setup/requirements.md) – Supported platforms and dependencies

### 🏗️ Architecture & Design

* [Runtime Behavior](docs/architecture/runtime_behavior.md) – Threading model, reconnection, backpressure
* [Memory Safety](docs/architecture/memory_safety.md) – Ownership rules and safety guarantees
* [System Overview](docs/architecture/system_overview.md) – High-level structure

### 🔧 Guides & Reference

* [API Reference](docs/reference/api_guide.md) – Public API overview
* [Performance Optimization](docs/guides/advanced/performance.md) – Build and runtime considerations
* [Testing Guide](docs/guides/core/testing.md) – Running tests and CI integration
* [Best Practices](docs/guides/core/best_practices.md) – Recommended usage patterns
* [Troubleshooting](docs/guides/core/troubleshooting.md) – Common issues and resolutions

### 💡 Examples & Tutorials

* [TCP Examples](examples/tcp/)
* [UDP Examples](examples/udp/)
* [Serial Examples](examples/serial/)
* [Tutorials](docs/tutorials/)
* [Documentation Index](docs/index.md)

---

## 📄 License

**unilink** is released under the Apache License, Version 2.0.

Commercial use, modification, and redistribution are permitted.
For details, see the [LICENSE](./LICENSE) and [NOTICE](./NOTICE) files.

Copyright © 2025 Jinwoo Sung
