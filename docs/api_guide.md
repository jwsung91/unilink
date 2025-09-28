# 📘 unilink API Guide

`unilink` is a unified, high-level asynchronous communication library. It provides a simple, fluent Builder API for various transport types, such as TCP (client/server) and Serial ports.

This guide explains how to use the library in your application.

---

## 1. Core Components

- **Builder API**: Fluent interface for creating and configuring communication channels
- **`unilink::builder::UnifiedBuilder`**: Main entry point for creating builders
- **`unilink::builder::TcpServerBuilder`**: Builder for TCP server channels
- **`unilink::builder::TcpClientBuilder`**: Builder for TCP client channels  
- **`unilink::builder::SerialBuilder`**: Builder for Serial port channels
- **Callbacks**: `std::function` objects for handling data, state changes, and errors

---

## 2. Quick Start with Builder API

The library provides a fluent Builder API that makes it easy to create and configure communication channels.

```cpp
#include "unilink/unilink.hpp"
#include <thread>
#include <chrono>

using namespace unilink;
using namespace std::chrono_literals;

int main() {
    // 1. Create a TCP client using Builder API
    auto client = unilink::builder::UnifiedBuilder::tcp_client("127.0.0.1", 9000)
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

    // 2. Send data
    std::string message = "Hello, Server!";
    client->send(message);

    // ... Application logic ...
    std::this_thread::sleep_for(10s);

    // 3. Stop the client (automatic cleanup)
    client->stop();

    return 0;
}
```

---

## 3. Builder API Examples

### TCP Server

```cpp
// Create a TCP server
auto server = unilink::builder::UnifiedBuilder::tcp_server(8080)
    .on_connect([]() {
        std::cout << "Client connected!" << std::endl;
    })
    .on_data([](const uint8_t* data, size_t len) {
        std::string message(data, data + len);
        std::cout << "Server received: " << message << std::endl;
    })
    .on_disconnect([]() {
        std::cout << "Client disconnected!" << std::endl;
    })
    .auto_start()
    .build();

// Send data to connected clients
server->send("Welcome to the server!");
```

### TCP Client

```cpp
// Create a TCP client
auto client = unilink::builder::UnifiedBuilder::tcp_client("127.0.0.1", 8080)
    .on_connect([]() {
        std::cout << "Connected to server!" << std::endl;
    })
    .on_data([](const uint8_t* data, size_t len) {
        std::string message(data, data + len);
        std::cout << "Client received: " << message << std::endl;
    })
    .on_error([](const std::string& error) {
        std::cerr << "Connection error: " << error << std::endl;
    })
    .auto_start()
    .build();

// Send data to server
client->send("Hello from client!");
```

### Serial Port

```cpp
// Create a Serial port connection
auto serial = unilink::builder::UnifiedBuilder::serial("/dev/ttyUSB0", 9600)
    .on_connect([]() {
        std::cout << "Serial port opened!" << std::endl;
    })
    .on_data([](const uint8_t* data, size_t len) {
        std::string message(data, data + len);
        std::cout << "Serial received: " << message << std::endl;
    })
    .on_error([](const std::string& error) {
        std::cerr << "Serial error: " << error << std::endl;
    })
    .auto_start()
    .build();

// Send data to serial port
serial->send("AT\r\n");
```

---

## 4. Advanced Builder Configuration

### Manual Control (without auto_start)

```cpp
auto client = unilink::builder::UnifiedBuilder::tcp_client("127.0.0.1", 8080)
    .on_connect([]() { std::cout << "Connected!" << std::endl; })
    .on_data([](const uint8_t* data, size_t len) {
        // Handle received data
    })
    .build();

// Manual start/stop control
client->start();
// ... do work ...
client->stop();
```

### Configuration Options

```cpp
auto server = unilink::builder::UnifiedBuilder::tcp_server(8080)
    .on_connect([]() { std::cout << "Client connected!" << std::endl; })
    .on_data([](const uint8_t* data, size_t len) {
        // Handle received data
    })
    .retry_interval(2000)  // 2 second retry interval
    .auto_manage()         // Automatic resource management
    .build();
```

### Convenience Functions

```cpp
// Short form builders
auto server = unilink::tcp_server(8080)
    .on_connect([]() { std::cout << "Connected!" << std::endl; })
    .auto_start()
    .build();

auto client = unilink::tcp_client("127.0.0.1", 8080)
    .on_data([](const uint8_t* data, size_t len) {
        std::string message(data, data + len);
        std::cout << "Received: " << message << std::endl;
    })
    .auto_start()
    .build();

auto serial = unilink::serial("/dev/ttyUSB0", 9600)
    .on_connect([]() { std::cout << "Serial opened!" << std::endl; })
    .auto_start()
    .build();
```

---

## 5. Threading Model

- All I/O callbacks run **inside a strand**, ensuring **order and no race conditions**
- The library manages its own background thread for I/O operations
- Your application can safely block on `std::future::wait()` without freezing I/O
- For multi-threaded data consumption, use thread-safe queues inside callbacks

---

## 6. Error Handling

- All errors are delivered via the `on_error` callback
- Automatic reconnection is available for client and serial connections
- Connection state changes are reported through `on_connect` and `on_disconnect` callbacks

---

## 7. Best Practices

### Resource Management

```cpp
// Use auto_manage() for automatic cleanup
auto client = unilink::tcp_client("127.0.0.1", 8080)
    .on_connect([]() { std::cout << "Connected!" << std::endl; })
    .auto_manage()  // Automatic resource management
    .build();
```

### Error Handling

```cpp
auto client = unilink::tcp_client("127.0.0.1", 8080)
    .on_error([](const std::string& error) {
        std::cerr << "Connection error: " << error << std::endl;
        // Handle error appropriately
    })
    .build();
```

### Data Processing

```cpp
auto client = unilink::tcp_client("127.0.0.1", 8080)
    .on_data([](const uint8_t* data, size_t len) {
        // Process data immediately or queue for later processing
        std::string message(data, data + len);
        process_message(message);
    })
    .build();
```

---

## 8. Migration from Low-Level API

If you were using the previous low-level API, here's how to migrate:

### Before (Low-Level API)
```cpp
TcpClientConfig cfg;
cfg.host = "127.0.0.1";
cfg.port = 8080;
auto client = unilink::create(cfg);
client->on_state([](LinkState state) { /* ... */ });
client->on_bytes([](const uint8_t* data, size_t len) { /* ... */ });
client->start();
```

### After (Builder API)
```cpp
auto client = unilink::tcp_client("127.0.0.1", 8080)
    .on_connect([]() { /* connected */ })
    .on_data([](const uint8_t* data, size_t len) { /* ... */ })
    .auto_start()
    .build();
```

---

## 9. FAQ

- **Q: Do I need to call read()?** → No, data arrives via `on_data` callback
- **Q: How are frames separated?** → You must implement your own framing logic
- **Q: Is it thread-safe?** → Yes, all I/O operations are thread-safe
- **Q: What about reconnection?** → Automatic reconnection is built-in for clients and serial ports
- **Q: Can I use multiple channels?** → Yes, create multiple builder instances
