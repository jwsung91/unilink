# unilink

<h3 align="center">One interface, reliable connections</h3>

`unilink` is a C++ library that provides a unified, low-level asynchronous communication interface for TCP (client/server) and Serial ports. It simplifies network and serial programming by abstracting transport-specific details behind a consistent `Channel` API.


---

## Features

- **Unified Interface**: A single `Channel` interface for TCP (Client/Server) and Serial communication.
- **Asynchronous Operations**: Callback-based, non-blocking I/O for high performance.
- **Automatic Reconnection**: Built-in, configurable reconnection logic for clients and serial ports.
- **Thread-Safe**: Managed I/O thread and thread-safe write operations.
- **Simple Lifecycle**: Easy-to-use `start()` and `stop()` methods for managing the connection lifecycle.

---

## Project Structure

```bash
.
├── unilink/         # Core library source and headers
├── examples/        # Example applications (see examples/README.md)
├── test/            # Unit tests
├── docs/            # Detailed API documentation
├── CMakeLists.txt   # Top-level build script
└── README.md        # This file
```

---

## Requirements

```bash
# For the core library
sudo apt update && sudo apt install -y \
  build-essential cmake libboost-dev libboost-system-dev
```

---

## How to Build

You can build the library by itself or include the examples.

```bash
# 1. Configure the project
# Build the library only
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Or, build the library and examples
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON

# Or, build the library and test
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON

# 2. Build the targets
cmake --build build -j
```
