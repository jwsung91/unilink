# Unilink Documentation Index

This index is the stable entry point for the handwritten documentation in `docs/`. It is intentionally kept lightweight so version bumps, option changes, and test-count changes do not require editing this page.

## Start Here

1. [Quick Start Guide](guides/core/quickstart.md)
2. [Installation Guide](guides/setup/installation.md)
3. [API Guide](reference/api_guide.md)
4. [Implementation Status](implementation_status.md)
5. [Build Guide](guides/setup/build_guide.md)

## Core Guides

| Document | What it covers |
|----------|----------------|
| [Build Guide](guides/setup/build_guide.md) | Actual CMake options and build profiles |
| [Requirements](guides/setup/requirements.md) | Platform and dependency expectations |
| [Best Practices](guides/core/best_practices.md) | Runtime usage patterns and guardrails |
| [Troubleshooting](guides/core/troubleshooting.md) | Common failures and debugging steps |
| [Testing](guides/core/testing.md) | Running tests and understanding coverage |
| [Python Bindings](guides/core/python_bindings.md) | `unilink` package usage and current scope |
| [Performance](guides/advanced/performance.md) | Tuning and async logging considerations |

## Tutorials

| Tutorial | Focus |
|----------|-------|
| [Getting Started](tutorials/01_getting_started.md) | First TCP client |
| [TCP Server](tutorials/02_tcp_server.md) | Server lifecycle and callbacks |
| [UDS Communication](tutorials/03_uds_communication.md) | Local IPC with Unix domain sockets |
| [Serial Communication](tutorials/04_serial_communication.md) | Device I/O and virtual-port testing |
| [UDP Communication](tutorials/05_udp_communication.md) | Connectionless send/receive workflow |

Tutorial companion material is split between `examples/tutorials/` and the protocol-specific example directories under `examples/serial/`, `examples/udp/`, and `examples/uds/`.

## Architecture Notes

| Document | Focus |
|----------|-------|
| [Architecture Overview](architecture/README.md) | Layers, responsibilities, design patterns |
| [Runtime Behavior](architecture/runtime_behavior.md) | Lifecycle, retries, callback behavior |
| [Memory Safety](architecture/memory_safety.md) | Ownership and buffer handling rules |
| [Channel Contract](architecture/channel_contract.md) | Transport-layer contract and stop semantics |
| [Wrapper Contract](architecture/wrapper_contract.md) | Wrapper lifecycle and callback guarantees |

## Examples and Tests

- [Examples Directory](../examples/README.md)
- [Tutorial Examples](../examples/tutorials/README.md)
- [Protocol Examples](../examples/tcp/README.md)
- [Serial Examples](../examples/serial/README.md)
- [UDP Examples](../examples/udp/README.md)
- [UDS Examples](../examples/uds/README.md)
- [Test Overview](test_structure.md)
- [Top-level Test Notes](../test/README.md)

## Notes For Maintainers

- Keep this page aligned with actual files under `docs/`.
- Prefer linking to existing files over placeholder topics.
- Do not duplicate release-specific values here. Version, build defaults, and test status belong in source-of-truth files or generated output.

[Back to Main README](../README.md)
