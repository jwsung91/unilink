# Unilink Documentation Index

Welcome to the comprehensive documentation for unilink - a modern C++ communication library.

---

## 📚 Documentation Structure

```
docs/
├── guides/
│   ├── setup/       # Installation and build guides
│   ├── core/        # Core concepts and usage guides
│   └── advanced/    # Performance and advanced topics
├── tutorials/       # Step-by-step tutorials
├── reference/       # API reference documentation
├── architecture/    # System design and architecture
└── development/     # Development and contribution guides
```

---

## 🚀 Getting Started

### New to Unilink?

1. **[Quick Start Guide](guides/core/quickstart.md)** - Get up and running in 5 minutes
2. **[Tutorial 1: Getting Started](tutorials/01_getting_started.md)** - Your first application
3. **[API Guide](reference/api_guide.md)** - Complete API reference

### Core Documentation

| Document | Description | Difficulty |
|----------|-------------|------------|
| [Quick Start](guides/core/quickstart.md) | 5-minute quick start | ⭐ Beginner |
| [API Guide](reference/api_guide.md) | Comprehensive API reference | ⭐⭐ All Levels |
| [Best Practices](guides/core/best_practices.md) | Recommended patterns | ⭐⭐ Intermediate |
| [System Overview](architecture/README.md) | Architecture deep-dive | ⭐⭐⭐ Advanced |

---

## 📖 Tutorials

Step-by-step guides for common tasks:

### Beginner Tutorials
- **[Tutorial 1: Getting Started](tutorials/01_getting_started.md)**
  - Install dependencies
  - Create your first client
  - Connect and send data
  - Duration: 15 minutes

- **[Tutorial 2: Building a TCP Server](tutorials/02_tcp_server.md)**
  - Create echo server
  - Handle multiple clients
  - Implement chat server
  - Duration: 20 minutes

### Coming Soon
- Tutorial 3: Serial Communication
- Tutorial 4: Error Handling & Recovery
- Tutorial 5: Performance Optimization
- Tutorial 6: Building Production Systems

---

## 📋 Guides

Practical guides for using unilink effectively:

### Setup Guides
| Guide | Topics Covered |
|-------|----------------|
| **[Installation](guides/setup/installation.md)** | Installing dependencies, CMake setup |
| **[Build Guide](guides/setup/build_guide.md)** | Build options, configurations |
| **[Requirements](guides/setup/requirements.md)** | System requirements |

### Core Guides
| Guide | Topics Covered |
|-------|----------------|
| **[Best Practices](guides/core/best_practices.md)** | Error handling, resource management, thread safety |
| **[Troubleshooting](guides/core/troubleshooting.md)** | Common issues, debugging tips, solutions |
| **[Testing](guides/core/testing.md)** | Testing strategies, unit tests |

### Advanced Guides
| Guide | Topics Covered |
|-------|----------------|
| **[Performance Guide](guides/advanced/performance.md)** | Optimization, tuning, benchmarks |

### Quick Reference

- **Error Handling**: [Best Practices §1](guides/core/best_practices.md#error-handling)
- **Thread Safety**: [Best Practices §3](guides/core/best_practices.md#thread-safety)
- **Performance**: [Performance Guide](guides/advanced/performance.md)
- **Debugging**: [Troubleshooting](guides/core/troubleshooting.md#debugging-tips)

---

## 📚 API Reference

Complete API documentation:

### Core APIs

| API | Description |
|-----|-------------|
| **[TCP Client](reference/api_guide.md#tcp-client)** | Connect to TCP servers |
| **[TCP Server](reference/api_guide.md#tcp-server)** | Accept client connections |
| **[Serial Communication](reference/api_guide.md#serial-communication)** | Interface with serial devices |
| **[Error Handling](reference/api_guide.md#error-handling)** | Centralized error management |
| **[Logging System](reference/api_guide.md#logging-system)** | Flexible logging |

### Advanced Features

- **[Configuration Management](reference/api_guide.md#configuration-management)** (Optional)
- **[Memory Pool](reference/api_guide.md#memory-pool)**
- **[Thread-Safe State](reference/api_guide.md#thread-safe-state)**
- **[Safe Data Buffer](reference/api_guide.md#safe-data-buffer)**

---

## 🏗️ Architecture

Understanding unilink's design:

### Architecture Documentation

| Document | Description |
|----------|-------------|
| **[System Overview](architecture/README.md)** | Complete architecture documentation |
| Design Patterns | Patterns used in unilink (Coming Soon) |
| Threading Model | Concurrency and thread safety (Coming Soon) |

### Key Concepts

1. **[Layered Architecture](architecture/README.md#layered-architecture)**
   - Builder API Layer
   - Wrapper API Layer
   - Transport Layer
   - Common Utilities Layer

2. **[Design Patterns](architecture/README.md#design-patterns)**
   - Builder Pattern
   - Observer Pattern
   - Dependency Injection
   - RAII

3. **[Threading Model](architecture/README.md#threading-model)**
   - IO Context Management
   - Thread Safety
   - Callback Execution

4. **[Memory Management](architecture/README.md#memory-management)**
   - Smart Pointers
   - Memory Pools
   - Safe Buffers

---

## 🔧 Development

For contributors and advanced users:

### Development Documentation

| Document | Description |
|----------|-------------|
| **[Improvement Recommendations](development/improvement_recommendations.md)** | Code quality assessment & roadmap |
| Contributing Guide | How to contribute (Coming Soon) |
| Code Style Guide | Coding standards (Coming Soon) |
| Testing Guide | Writing and running tests (Coming Soon) |

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | `Release` | Build type (Release/Debug) |
| `UNILINK_BUILD_EXAMPLES` | `ON` | Build example applications |
| `UNILINK_BUILD_TESTS` | `ON` | Master test toggle |
| `UNILINK_ENABLE_PERFORMANCE_TESTS` | `OFF` | Build performance/benchmark tests |
| `UNILINK_ENABLE_CONFIG` | `ON` | Enable configuration API |
| `UNILINK_ENABLE_MEMORY_TRACKING` | `ON` | Enable memory tracking |
| `UNILINK_ENABLE_SANITIZERS` | `OFF` | Enable AddressSanitizer |

---

## 💡 Examples

Practical code examples:

### Example Applications

Located in `examples/` directory:

| Example | Description | Lines of Code |
|---------|-------------|---------------|
| **TCP Echo Server** | Simple echo server | ~100 |
| **TCP Echo Client** | Echo client with reconnection | ~80 |
| **Serial Echo** | Serial port echo | ~90 |
| **Chat Server** | Multi-client chat server | ~200 |
| **Logging Example** | Logging system demo | ~60 |
| **Error Handling Example** | Error handling demo | ~80 |

### Code Snippets

**Quick TCP Client:**
```cpp
auto client = unilink::tcp_client("127.0.0.1", 8080)
    .on_data([](const std::string& data) {
        std::cout << "Received: " << data << std::endl;
    })
    .build();

client->start();
```

**Quick TCP Server:**
```cpp
auto server = unilink::tcp_server(8080)
    .unlimited_clients()
    .on_data([](size_t id, const std::string& data) {
        std::cout << "Client " << id << ": " << data << std::endl;
    })
    .build();

server->start();
// ... do work ...
server->stop();  // Clean shutdown
```

---

## 🔍 Search & Find

### By Topic

- **Connection Issues**: [Troubleshooting §1](guides/core/troubleshooting.md#connection-issues)
- **Performance Problems**: [Troubleshooting §4](guides/core/troubleshooting.md#performance-issues)
- **Memory Leaks**: [Troubleshooting §5](guides/core/troubleshooting.md#memory-issues)
- **Thread Safety**: [Best Practices §3](guides/core/best_practices.md#thread-safety)
- **Error Handling**: [Best Practices §1](guides/core/best_practices.md#error-handling)

### By Use Case

| Use Case | Relevant Documentation |
|----------|------------------------|
| IoT Device | [Serial API](reference/api_guide.md#serial-communication), [Performance](guides/advanced/performance.md) |
| Web Service | [TCP Server](reference/api_guide.md#tcp-server), [Best Practices](guides/core/best_practices.md) |
| Data Streaming | [Performance Guide](guides/advanced/performance.md), [Architecture](architecture/README.md) |
| Testing | [Troubleshooting](guides/core/troubleshooting.md), [API Guide](reference/api_guide.md) |

---

## 📊 Documentation Stats

| Metric | Count |
|--------|-------|
| Total Pages | 10+ |
| Tutorials | 2 (more coming) |
| Guides | 7 |
| Code Examples | 50+ |
| API Methods Documented | 100+ |

---

## 🆘 Need Help?

### Quick Links

- **Common Issues**: [Troubleshooting Guide](guides/core/troubleshooting.md)
- **FAQ**: [Troubleshooting §Getting Help](guides/core/troubleshooting.md#getting-help)
- **Examples**: [Examples Directory](../examples/)
- **GitHub Issues**: [Report a Bug](https://github.com/jwsung91/unilink/issues)

### Learning Path

**Beginner Path:**
1. Quick Start Guide
2. Tutorial 1: Getting Started
3. Tutorial 2: TCP Server
4. API Guide (TCP sections)
5. Best Practices (Error Handling)

**Intermediate Path:**
1. API Guide (complete)
2. Best Practices (complete)
3. Performance Guide
4. Architecture Overview

**Advanced Path:**
1. System Architecture
2. Threading Model
3. Memory Management
4. Contributing Guide

---

## 📝 Document History

| Date | Version | Changes |
|------|---------|---------|
| 2025-10-11 | 1.0 | Initial comprehensive documentation release |
| 2025-12-27 | 1.1 | Restructured documentation (Categories, snake_case) |

---

## 🤝 Contributing

Found an issue or want to improve documentation?

1. **Report Issues**: [GitHub Issues](https://github.com/jwsung91/unilink/issues)
2. **Suggest Improvements**: Create a pull request
3. **Ask Questions**: Create an issue with "question" label

---

**Happy Coding with Unilink!** 🚀

[Back to Main README](../README.md)