# unilink

[![CI/CD Pipeline](https://github.com/grade-e/interface-socket/actions/workflows/ci.yml/badge.svg)](https://github.com/grade-e/interface-socket/actions/workflows/ci.yml)
[![Test Coverage](https://img.shields.io/endpoint?url=https://grade-e.github.io/interface-socket/badges/coverage.json)](https://grade-e.github.io/interface-socket/)
![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)

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

---

## 🚀 Quick Start

### 30-Second Example

```cpp
#include "unilink/unilink.hpp"

int main() {
    // Create a TCP client - it's that simple!
    auto client = unilink::tcp_client("127.0.0.1", 8080)
        .on_connect([]() { std::cout << "Connected!" << std::endl; })
        .on_data([](const std::string& data) { std::cout << "Received: " << data << std::endl; })
        .auto_start(true)
        .build();
    
    client->send("Hello, Server!");
    std::this_thread::sleep_for(std::chrono::seconds(5));
}
```

### Installation

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt update && sudo apt install -y build-essential cmake libboost-dev libboost-system-dev

# Build the library
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# Compile your application
g++ -std=c++17 my_app.cc -lunilink -lboost_system -pthread -o my_app
./my_app
```

**For detailed installation instructions, see [Build Guide](docs/guides/build_guide.md).**

---

## 📚 Documentation

### 🚦 Getting Started
- [Quick Start Guide](docs/guides/QUICKSTART.md) - Get up and running in 5 minutes
- [Installation & Build](docs/guides/build_guide.md) - Detailed build instructions and options
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

---

**Built with ❤️ using Modern C++17 and Boost.Asio**
