# Tutorial Examples

Ready-to-compile examples that mirror the tutorial documentation.

## Included Examples

### Getting Started

| Example | File | Description | Tutorial Link |
|---------|------|-------------|---------------|
| **Simple Client** | `simple_client.cpp` | Minimal one-shot client | [Getting Started](../../docs/tutorials/01_getting_started.md) |
| **My First Client** | `my_first_client.cpp` | Interactive client with callbacks and retries | [Getting Started](../../docs/tutorials/01_getting_started.md) |

### TCP Server

| Example | File | Description | Tutorial Link |
|---------|------|-------------|---------------|
| **Echo Server** | `echo_server.cpp` | Fixed-port echo server on `8080` | [TCP Server](../../docs/tutorials/02_tcp_server.md) |
| **Chat Server** | `chat_server.cpp` | Fixed-port multi-client chat server on `8080` | [TCP Server](../../docs/tutorials/02_tcp_server.md) |

## Quick Start

### Build All Tutorial Examples

```bash
cd build
cmake --build . --target tutorial_simple_client tutorial_my_first_client tutorial_echo_server tutorial_chat_server
```

### Run Examples

#### Simple Client

```bash
# Terminal 1
nc -l 8080

# Terminal 2
./build/bin/tutorial_simple_client
```

`tutorial_simple_client` connects once, sends `Hello, Unilink!`, and exits.

#### My First Client

```bash
# Terminal 1
nc -l 8080

# Terminal 2
./build/bin/tutorial_my_first_client

# Custom target
./build/bin/tutorial_my_first_client 192.168.1.100 9000
```

#### Echo Server

```bash
# Terminal 1
./build/bin/tutorial_echo_server

# Terminal 2
nc localhost 8080
```

#### Chat Server

```bash
# Terminal 1
./build/bin/tutorial_chat_server

# Terminal 2
telnet localhost 8080

# Terminal 3
telnet localhost 8080
```

Available commands inside the chat example:

```text
/nick Alice
/list
/help
/quit
Hello everyone!
```

## Additional Protocol Tutorials

Not every tutorial has dedicated files under `examples/tutorials/`.

- Serial tutorial examples live under [examples/serial/README.md](../../examples/serial/README.md)
- UDP tutorial examples live under [examples/udp/README.md](../../examples/udp/README.md)
- UDS tutorial examples live under [examples/uds/README.md](../../examples/uds/README.md)

## Notes

- Tutorial binaries are emitted under `build/bin/` in the current build setup.
- `tutorial_echo_server` and `tutorial_chat_server` are fixed to port `8080`; they do not parse CLI port arguments.
- All tutorial examples use the public `unilink/unilink.hpp` API.
