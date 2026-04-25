# Contributor Guide {#contrib_index}

Documentation for developers building, extending, or contributing to unilink itself.

---

## Building the Library

- [Build Guide](build_guide.md) — CMake options, build profiles, sanitizers, platform-specific notes
- [Testing](testing.md) — Running the test suite, CI integration, writing tests
- [Implementation Status](implementation_status.md) — Verified scope, known gaps, and codebase snapshot
- [Test Structure](test_structure.md) — Test organization and coverage breakdown

---

## Architecture

Internal design and contracts that all transport implementations must follow:

| Document | What it covers |
|----------|----------------|
| [Architecture Overview](architecture/README.md) | Layers, responsibilities, design patterns |
| [Runtime Behavior](architecture/runtime_behavior.md) | Lifecycle, retries, backpressure, callback behavior |
| [Memory Safety](architecture/memory_safety.md) | Ownership rules and buffer handling guarantees |
| [Channel Contract](architecture/channel_contract.md) | Transport-layer stop semantics and state transitions |
| [Wrapper Contract](architecture/wrapper_contract.md) | Wrapper lifecycle and callback guarantees |

---

## Quick Links (User Docs)

- [API Reference](../user/api_guide.md)
- [Troubleshooting](../user/troubleshooting.md)

---

[← Documentation Index](../index.md)
