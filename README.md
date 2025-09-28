# unilink

## One interface, reliable connections

`unilink` is a C++ library that provides a unified, high-level Builder API for TCP (client/server) and Serial ports. It simplifies network and serial programming with a fluent, easy-to-use interface that handles all the complexity behind the scenes.

---

## Features

- **Builder API**: Fluent, chainable interface for creating and configuring communication channels
- **Unified Interface**: Single API for TCP (Client/Server) and Serial communication
- **Asynchronous Operations**: Callback-based, non-blocking I/O for high performance
- **Automatic Reconnection**: Built-in, configurable reconnection logic for clients and serial ports
- **Thread-Safe**: Managed I/O thread and thread-safe operations
- **Auto Management**: Optional automatic resource management and lifecycle control
- **Modular Design**: Optional configuration management API for advanced users
- **Optimized Builds**: Configurable compilation for minimal footprint

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

## Usage

### Basic Usage (Builder API)

Most users will only need the Builder API for simple communication:

```cpp
#include "unilink/unilink.hpp"

// TCP Client
auto client = unilink::tcp_client("127.0.0.1", 9000)
    .on_connect([]() { std::cout << "Connected!" << std::endl; })
    .on_data([](const std::string& data) { std::cout << "Received: " << data << std::endl; })
    .auto_start()
    .build();

// TCP Server
auto server = unilink::tcp_server(9000)
    .on_connect([]() { std::cout << "Client connected!" << std::endl; })
    .on_data([](const std::string& data) { std::cout << "Received: " << data << std::endl; })
    .auto_start()
    .build();

// Serial Communication
auto serial = unilink::serial("/dev/ttyUSB0", 115200)
    .on_data([](const std::string& data) { std::cout << "Received: " << data << std::endl; })
    .auto_start()
    .build();
```

### Advanced Usage (Configuration Management API)

For advanced users who need dynamic configuration:

```cpp
#include "unilink/unilink.hpp"

// Enable configuration management (requires UNILINK_ENABLE_CONFIG=ON)
auto config = unilink::config_manager::ConfigFactory::create_with_defaults();

// Configure TCP client settings
config->set("tcp.client.host", std::string("127.0.0.1"));
config->set("tcp.client.port", 9000);
config->set("tcp.client.retry_interval_ms", 1000);

// Configure serial settings
config->set("serial.port", std::string("/dev/ttyUSB0"));
config->set("serial.baud_rate", 115200);

// Save configuration to file
config->save_to_file("my_config.conf");

// Load configuration from file
auto loaded_config = unilink::config_manager::ConfigFactory::create_from_file("my_config.conf");
```

### Build Configuration

Choose the build configuration that fits your needs:

- **Minimal Build** (`UNILINK_ENABLE_CONFIG=OFF`): Only Builder API, smaller footprint
- **Full Build** (`UNILINK_ENABLE_CONFIG=ON`): Includes configuration management API

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

You can build the library with different configurations to optimize for your use case.

### Basic Build (Builder API only - recommended for most users)

```bash
# 1. Configure for minimal footprint (Builder API only)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUNILINK_ENABLE_CONFIG=OFF

# 2. Build the targets
cmake --build build -j
```

### Full Build (includes Configuration Management API)

```bash
# 1. Configure with all features
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUNILINK_ENABLE_CONFIG=ON

# 2. Build the targets
cmake --build build -j
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `UNILINK_ENABLE_CONFIG` | `ON` | Enable configuration management API |
| `BUILD_EXAMPLES` | `ON` | Build example applications |
| `BUILD_TESTING` | `ON` | Build unit tests |

### Examples

```bash
# Build library and examples (minimal)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUNILINK_ENABLE_CONFIG=OFF -DBUILD_EXAMPLES=ON

# Build library and tests (full features)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUNILINK_ENABLE_CONFIG=ON -DBUILD_TESTING=ON

# Build everything
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUNILINK_ENABLE_CONFIG=ON -DBUILD_EXAMPLES=ON -DBUILD_TESTING=ON
```
