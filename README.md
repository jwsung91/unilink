# unilink

[![CI/CD Pipeline](https://github.com/jwsung91/unilink/actions/workflows/ci.yml/badge.svg)](https://github.com/jwsung91/unilink/actions/workflows/ci.yml)
[![Test Coverage](https://img.shields.io/endpoint?url=https://jwsung91.github.io/unilink/badges/coverage.json)](https://github.com/jwsung91/unilink)

![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows%20%7C%20Mac-Green)

![License](https://img.shields.io/badge/License-Apache_2.0-lightgray.svg)

## One interface, reliable connections

`unilink` is a modern C++17 library that provides a unified, high-level Builder API for TCP (client/server) and Serial port communication. It simplifies network and serial programming with a fluent, easy-to-use interface that handles all the complexity behind the scenes.

---

## ✨ Features

- 🔌 **Unified API** - Single interface for TCP (Client/Server) and Serial communication
- 🔄 **Automatic Reconnection** - Built-in, configurable reconnection logic
- 🧵 **Thread-Safe** - All operations are thread-safe with managed I/O threads
- 🛡️ **Memory Safety** - Comprehensive bounds checking, leak detection, and safe data handling
- ⚡ **High Performance** - Asynchronous I/O with callback-based non-blocking operations
- 🎯 **Modern C++17** - Clean, fluent Builder API with optional configuration management
- ✅ **Well-Tested** - 72.2% test coverage with comprehensive unit, integration, and E2E tests
- 🚀 **CI/CD Ready** - Automated testing on Ubuntu 22.04 and 24.04 with build verification for Ubuntu 20.04

---

## 📚 Documentation

### 🚦 Getting Started
- [Quick Start Guide](docs/guides/QUICKSTART.md) - Get up and running in 5 minutes
- [Installation Guide](docs/guides/installation.md) - CMake package, source build, and release packages
- [Build Guide](docs/guides/build_guide.md) - Detailed build instructions and options
- [Requirements](docs/guides/requirements.md) - System requirements and dependencies

### 🏗️ Architecture & Design  
- [Runtime Behavior](docs/architecture/runtime_behavior.md) - Threading model, reconnection policies, backpressure
- [Memory Safety](docs/architecture/memory_safety.md) - Safety features and guarantees
- [System Overview](docs/architecture/system_overview.md) - High-level architecture

### 🔧 Guides & Reference
- [API Reference](docs/reference/API_GUIDE.md) - Comprehensive API documentation
- [Performance Optimization](docs/guides/performance.md) - Build configurations and optimization
- [Testing Guide](docs/guides/testing.md) - Running tests and CI/CD integration
- [Best Practices](docs/guides/best_practices.md) - Recommended patterns and usage
- [Troubleshooting](docs/guides/troubleshooting.md) - Common issues and solutions
- [Release Notes](docs/releases/) - Version history and migration guides

### 💡 Examples & Tutorials
- [TCP Examples](examples/tcp/) - Client/Server examples
- [Serial Examples](examples/serial/) - Serial port communication
- [Tutorials](docs/tutorials/) - Step-by-step learning guides
- [Documentation Index](docs/INDEX.md) - Complete documentation overview

---

## 📄 License

**unilink** is released under the [Apache License, Version 2.0](./LICENSE), which allows commercial use, modification, distribution, and patent use.

**Copyright © 2025 Jinwoo Sung**

This project was independently developed by Jinwoo Sung and is **not affiliated with any employer**.

Please note that some third-party dependencies use different licenses:
- **Boost** (Boost Software License 1.0) - Permissive, compatible with Apache 2.0

For full license details and attribution requirements, see the [LICENSE](./LICENSE) and [NOTICE](./NOTICE) files.
