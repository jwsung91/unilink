# unilink

[![CI](https://github.com/jwsung91/unilink/actions/workflows/ci.yml/badge.svg)](https://github.com/jwsung91/unilink/actions/workflows/ci.yml)
[![Coverage](https://img.shields.io/endpoint?url=https://jwsung91.github.io/unilink/badges/coverage.json)](https://github.com/jwsung91/unilink)

![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows%20%7C%20macOS-informational)
![vcpkg](https://img.shields.io/badge/vcpkg-jwsung91--unilink-0078D6)

## One interface, reliable connections

`unilink` is a modern C++17 library that provides a unified, high-level Builder API for TCP (client/server), UDP, and Serial port communication. It simplifies network and serial programming with a fluent, easy-to-use interface that handles all the complexity behind the scenes.

### UDP in the same API

```cpp
#include "unilink/unilink.hpp"

int main() {
  auto channel = unilink::udp(9000)
                     .set_remote("127.0.0.1", 9001)  // Optional: can also learn the peer from first packet
                     .on_data([](const std::string& data) { /* handle data */ })
                     .build();

  channel->start();
  channel->send("hello over udp");
}
```

- Uses Boost.Asio async `send_to`/`receive_from` under the hood
- Message boundaries are preserved (datagram semantics)
- No connection handshake; `Connected` state simply means the socket is bound and a peer is known

## 📦 Installation

### vcpkg (recommended)

```bash
vcpkg install jwsung91-unilink
```

> For CMake usage, source builds, and other installation options,
> see the [Installation Guide](docs/guides/installation.md).

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

## 📄 License

**unilink** is released under the [Apache License, Version 2.0](./LICENSE), which allows commercial use, modification, distribution, and patent use.

**Copyright © 2025 Jinwoo Sung**

For full license details and attribution requirements, see the [LICENSE](./LICENSE) and [NOTICE](./NOTICE) files.
