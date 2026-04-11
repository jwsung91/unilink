# Tutorial Examples

Ready-to-compile example sources that mirror the tutorial documentation.

## Included Examples

| Area | File | Tutorial |
|------|------|----------|
| Getting Started | `getting_started/simple_client.cpp` | [01_getting_started.md](../../docs/tutorials/01_getting_started.md) |
| Getting Started | `getting_started/my_first_client.cpp` | [01_getting_started.md](../../docs/tutorials/01_getting_started.md) |
| TCP Server | `tcp_server/echo_server.cpp` | [02_tcp_server.md](../../docs/tutorials/02_tcp_server.md) |
| TCP Server | `tcp_server/chat_server.cpp` | [02_tcp_server.md](../../docs/tutorials/02_tcp_server.md) |

## Build And Run

- Build and binary-path details: [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md)
- Tutorial walkthroughs: [docs/tutorials/](../../docs/tutorials/)

## Related Protocol Examples

Not every tutorial has a dedicated file under `examples/tutorials/`.

- Serial examples: [../serial/README.md](../serial/README.md)
- UDP examples: [../udp/README.md](../udp/README.md)
- UDS examples: [../uds/README.md](../uds/README.md)

## Notes

- This file is intentionally an index to reduce duplication with the tutorial docs and `BUILD_INSTRUCTIONS.md`.
- Tutorial binaries are emitted under `build/bin/` in the current build setup.
- All tutorial examples use the public `unilink/unilink.hpp` API.
