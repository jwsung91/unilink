# Unilink Documentation {#docs_index}

Choose the guide that matches your role.

---

## 📖 For Library Users

You are building an application using unilink.

→ **[User Guide](user/index.md)**

| Document | What it covers |
|----------|----------------|
| [Quick Start](user/quickstart.md) | First working client in minutes |
| [Installation](user/installation.md) | vcpkg, source, release packages |
| [Requirements](user/requirements.md) | Platform and dependency expectations |
| [API Reference](user/api_guide.md) | Full public API: builders, wrappers, callbacks |
| [Troubleshooting](user/troubleshooting.md) | Common failures and debugging steps |
| [Python Bindings](user/python_bindings.md) | Python API guide |
| [Performance](user/performance.md) | Build and runtime tuning |

**Tutorials:**

| Tutorial | Focus |
|----------|-------|
| [Getting Started](user/tutorials/01_getting_started.md) | First TCP client |
| [TCP Server](user/tutorials/02_tcp_server.md) | Server lifecycle and callbacks |
| [UDS Communication](user/tutorials/03_uds_communication.md) | Local IPC with Unix domain sockets |
| [Serial Communication](user/tutorials/04_serial_communication.md) | Device I/O and virtual-port testing |
| [UDP Communication](user/tutorials/05_udp_communication.md) | Connectionless send/receive workflow |

---

## 🔧 For Contributors

You are developing or extending unilink itself.

→ **[Contributor Guide](contributor/index.md)**

| Document | What it covers |
|----------|----------------|
| [Build Guide](contributor/build_guide.md) | CMake options, build profiles, sanitizers |
| [Testing](contributor/testing.md) | Running tests, CI integration |
| [Implementation Status](contributor/implementation_status.md) | Verified scope and known gaps |
| [Test Structure](contributor/test_structure.md) | Test organization and coverage |
| [Architecture Overview](contributor/architecture/README.md) | Layers, responsibilities, design patterns |
| [Runtime Behavior](contributor/architecture/runtime_behavior.md) | Lifecycle, retries, callback behavior |
| [Memory Safety](contributor/architecture/memory_safety.md) | Ownership and buffer handling rules |
| [Channel Contract](contributor/architecture/channel_contract.md) | Transport-layer contract and stop semantics |
| [Wrapper Contract](contributor/architecture/wrapper_contract.md) | Wrapper lifecycle and callback guarantees |

---

## Examples and Tests

- [Examples Directory](../examples/README.md)
- [Protocol Examples](../examples/tcp/README.md)
- [Serial Examples](../examples/serial/README.md)
- [UDP Examples](../examples/udp/README.md)
- [UDS Examples](../examples/uds/README.md)
- [Top-level Test Notes](../test/README.md)

[Back to Main README](../README.md)
