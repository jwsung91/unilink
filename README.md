# unilink

<h3 align="center">One interface, reliable connections</h3>

`unilink` is a C++ library that provides a unified, high-level Builder API for TCP (client/server) and Serial ports. It simplifies network and serial programming with a fluent, easy-to-use interface that handles all the complexity behind the scenes.

---

## Features

- **Builder API**: Fluent, chainable interface for creating and configuring communication channels
- **Unified Interface**: Single API for TCP (Client/Server) and Serial communication
- **Asynchronous Operations**: Callback-based, non-blocking I/O for high performance
- **Automatic Reconnection**: Built-in, configurable reconnection logic for clients and serial ports
- **Thread-Safe**: Managed I/O thread and thread-safe operations
- **Auto Management**: Optional automatic resource management and lifecycle control

---

## Quick Start

```cpp
#include "unilink/unilink.hpp"

int main() {
    // Create a TCP client with Builder API
    auto client = unilink::tcp_client("127.0.0.1", 8080)
        .on_connect([]() {
            std::cout << "Connected to server!" << std::endl;
        })
        .on_data([](const uint8_t* data, size_t len) {
            std::string message(data, data + len);
            std::cout << "Received: " << message << std::endl;
        })
        .on_error([](const std::string& error) {
            std::cerr << "Error: " << error << std::endl;
        })
        .auto_start()  // Automatically start the connection
        .build();

    // Send data
    client->send("Hello, Server!");

    // Keep running
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Automatic cleanup when client goes out of scope
    return 0;
}
```

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
