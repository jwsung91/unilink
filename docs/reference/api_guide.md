# Unilink API Guide

Comprehensive API reference for the unilink library.

---

## Table of Contents

1. [Builder API](#builder-api)
2. [TCP Client](#tcp-client)
3. [TCP Server](#tcp-server)
4. [Serial Communication](#serial-communication)
5. [Error Handling](#error-handling)
6. [Logging System](#logging-system)
7. [Configuration Management](#configuration-management)
8. [Advanced Features](#advanced-features)

---

## Builder API

The Builder API is the recommended way to use unilink. It provides a fluent, chainable interface for creating communication channels.

### Core Concept

```cpp
auto channel = unilink::{type}(params)
    .option1(value1)
    .option2(value2)
    .on_event(callback)
    .build();
```

### Common Methods (All Builders)

| Method                           | Description                                                       | Default |
| -------------------------------- | ----------------------------------------------------------------- | ------- |
| `.on_data(callback)`             | Handle incoming data                                              | None    |
| `.on_connect(callback)`          | Handle connection events                                          | None    |
| `.on_disconnect(callback)`       | Handle disconnection                                              | None    |
| `.on_error(callback)`            | Handle errors                                                     | None    |
| `.auto_manage(bool)`             | Auto-start/stop the wrapper (starts immediately when `true`)      | `false` |
| `.use_independent_context(bool)` | Create and run a dedicated `io_context` thread managed by unilink | `false` |
| `.build()`                       | **Required**: Build the wrapper instance                          | -       |

**Builder-Specific Options**

- `TcpClientBuilder` / `SerialBuilder`: `.retry_interval(ms)` (default `3000ms`)
- `TcpServerBuilder`: `.enable_port_retry(enable, max_retries, retry_interval_ms)`
- `TcpServerBuilder`: `.single_client()`, `.multi_client(max>=2)`, `.unlimited_clients()` **(must choose one before `build()`)**
- TCP server callbacks also accept multi-client signatures: `.on_connect(size_t, std::string)`, `.on_data(size_t, std::string)`, `.on_disconnect(size_t)`

**Lifecycle Methods:**
| Method | Description |
| ------------------------------------ | ---------------------------------------------------------------------- |
| `->start()` | **Required**: Start the connection |
| `->stop()` | Stop the connection |
| `->send(data)` / `->send_line(text)` | Send data to peer(s) |
| `->is_connected()` | Check connection status (`TcpServer` reports underlying channel state) |
| `TcpServer::is_listening()` | Check if the server socket is bound and listening |

**Builder Flow**

```
tcp_server(port)
    ↓ configure callbacks / limits / port retry
    build()  → std::unique_ptr<wrapper::TcpServer>
                 ↓
                 start()
                 ↓ callbacks fire: on_connect → on_data → on_disconnect/on_error
```

### IO Context Ownership (advanced)

- **Default**: Builders use the shared `IoContextManager` thread; unilink starts/stops it for you.
- **`use_independent_context(true)`**: Builder creates its own `io_context` and runs it on an internal thread; cleanup is automatic.
- **External `io_context`**: If you manually pass a custom `io_context` to wrapper constructors, unilink will _not_ run/stop it unless you call `set_manage_external_context(true)` on the wrapper. In that case, callbacks should be registered before enabling `auto_manage(true)` (it starts immediately).

---

## TCP Client

Connect to remote TCP servers with automatic reconnection.

### Basic Usage

```cpp
#include "unilink/unilink.hpp"

auto client = unilink::tcp_client("192.168.1.100", 8080)
    .on_connect([]() {
        std::cout << "Connected!" << std::endl;
    })
    .on_data([](const std::string& data) {
        std::cout << "Received: " << data << std::endl;
    })
    .on_disconnect([]() {
        std::cout << "Disconnected" << std::endl;
    })
    .on_error([](const std::string& error) {
        std::cerr << "Error: " << error << std::endl;
    })
    .retry_interval(3000)  // Optional: Retry every 3 seconds (default)
    .build();

// Start connection
client->start();

// Send data
if (client->is_connected()) {
    client->send("Hello, Server!");
}

// Stop when done
client->stop();
```

### API Reference

#### Constructor

```cpp
unilink::tcp_client(const std::string& host, uint16_t port)
```

#### Builder Methods

| Method                      | Parameters | Description                                                |
| --------------------------- | ---------- | ---------------------------------------------------------- |
| `retry_interval(ms)`        | `unsigned` | Set reconnection interval in milliseconds (default `3000`) |
| `use_independent_context()` | `bool`     | Use separate IO thread (for testing)                       |
| `auto_manage()`             | `bool`     | Auto-start immediately and stop on destruction             |

#### Instance Methods

| Method                     | Return | Description                                                           |
| -------------------------- | ------ | --------------------------------------------------------------------- |
| `send()`                   | `void` | Send data to server                                                   |
| `send_line()`              | `void` | Send data with trailing newline                                       |
| `is_connected()`           | `bool` | Check connection status                                               |
| `start()`                  | `void` | Start connection attempt                                              |
| `stop()`                   | `void` | Stop and disconnect                                                   |
| `set_retry_interval()`     | `void` | Adjust reconnection interval at runtime (`std::chrono::milliseconds`) |
| `set_max_retries()`        | `void` | Set max reconnect attempts (`-1` for unlimited)                       |
| `set_connection_timeout()` | `void` | Set connection timeout (`std::chrono::milliseconds`)                  |

### Advanced Examples

#### With Member Functions

```cpp
class MyClient {
    std::shared_ptr<unilink::wrapper::TcpClient> client_;

public:
    void on_data(const std::string& data) {
        // Handle data
    }

    void connect() {
        client_ = unilink::tcp_client("server.com", 8080)
            .on_data(this, &MyClient::on_data)  // Member function!
            .on_connect(this, &MyClient::on_connect)
            .build();
    }
};
```

#### With Lambda Capture

```cpp
std::string device_id = "sensor_001";
auto client = unilink::tcp_client("127.0.0.1", 8080)
    .on_data([device_id](const std::string& data) {
        std::cout << "[" << device_id << "] " << data << std::endl;
    })
    .build();
```

---

## TCP Server

Accept multiple client connections with thread-safe operations.

### Basic Usage

```cpp
#include "unilink/unilink.hpp"

auto server = unilink::tcp_server(8080)
    .on_connect([](size_t client_id, const std::string& ip) {
        std::cout << "Client " << client_id << " connected from " << ip << std::endl;
    })
    .on_data([](size_t client_id, const std::string& data) {
        std::cout << "Client " << client_id << ": " << data << std::endl;
    })
    .on_disconnect([](size_t client_id) {
        std::cout << "Client " << client_id << " disconnected" << std::endl;
    })
    .build();

// Start server
server->start();

// Send to specific client
server->send_to_client(1, "Hello, Client 1!");

// Send to all clients
server->send("Broadcast message");

// Check if listening
if (server->is_listening()) {
    std::cout << "Server is listening" << std::endl;
}

// Clean shutdown
server->stop();
```

> Note: `TcpServerBuilder` requires an explicit client limit before `build()` is called. Choose one of
> `.single_client()`, `.multi_client(max)`, or `.unlimited_clients()` during construction.

### API Reference

#### Constructor

```cpp
unilink::tcp_server(uint16_t port)
```

#### Builder Methods

| Method                      | Parameters                   | Description                                                          |
| --------------------------- | ---------------------------- | -------------------------------------------------------------------- |
| `single_client()`           | None                         | Accept only one client (required to choose a limit before `build()`) |
| `multi_client(max)`         | `size_t (>=2)`               | Accept up to `max` clients                                           |
| `unlimited_clients()`       | None                         | Accept unlimited clients                                             |
| `enable_port_retry()`       | `bool, retries, interval_ms` | Retry if port is in use                                              |
| `use_independent_context()` | `bool`                       | Run on a dedicated `io_context` thread managed by unilink            |
| `auto_manage()`             | `bool`                       | Auto-start immediately and stop on destruction                       |

Multi-client callbacks can be registered with either signature-based overloads (`.on_connect(size_t, std::string)`,
`.on_data(size_t, std::string)`, `.on_disconnect(size_t)`) or the explicit helpers
`.on_multi_connect`, `.on_multi_data`, and `.on_multi_disconnect` (available on both the builder and the wrapper).

#### Instance Methods

| Method                    | Return                | Description                     |
| ------------------------- | --------------------- | ------------------------------- |
| `send()`                  | `void`                | Send to all connected clients   |
| `send_to_client()`        | `void`                | Send to specific client         |
| `broadcast()`             | `void`                | Explicit broadcast helper       |
| `send_line()`             | `void`                | Send data with trailing newline |
| `get_client_count()`      | `size_t`              | Number of connected clients     |
| `get_connected_clients()` | `std::vector<size_t>` | List of connected client IDs    |
| `is_connected()`          | `bool`                | Channel connection state        |
| `is_listening()`          | `bool`                | Check if server is listening    |
| `start()`                 | `void`                | Start accepting connections     |
| `stop()`                  | `void`                | Stop server and disconnect all  |

### Advanced Examples

#### Single Client Mode

```cpp
auto server = unilink::tcp_server(8080)
    .single_client()  // Only one client allowed
    .on_connect([](size_t client_id, const std::string& ip) {
        std::cout << "Client connected: " << ip << std::endl;
    })
    .build();
```

#### Port Retry

```cpp
auto server = unilink::tcp_server(8080)
    .enable_port_retry(true, 5, 1000)  // 5 retries, 1 second each
    .on_error([](const std::string& error) {
        std::cerr << "Server error: " << error << std::endl;
    })
    .build();
```

#### Echo Server Pattern

```cpp
auto server = unilink::tcp_server(8080)
    .unlimited_clients()
    .build();

server->on_multi_data([&server](size_t client_id, const std::string& data) {
    server->send_to_client(client_id, "Echo: " + data);
});

server->start();
```

---

## Serial Communication

Interface with serial devices and embedded systems.

### Basic Usage

```cpp
#include "unilink/unilink.hpp"

auto serial = unilink::serial("/dev/ttyUSB0", 115200)
    .on_connect([]() {
        std::cout << "Serial port opened" << std::endl;
    })
    .on_data([](const std::string& data) {
        std::cout << "Received: " << data << std::endl;
    })
    .build();

// Start serial communication
serial->start();

// Send AT command
serial->send("AT\r\n");

// Send binary data
std::vector<uint8_t> binary = {0x01, 0x02, 0x03};
serial->send(binary);
```

### API Reference

#### Constructor

```cpp
unilink::serial(const std::string& device, uint32_t baud_rate)
```

**Common Baud Rates:**

- 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600

#### Builder Methods

| Method                      | Parameters | Description                                               |
| --------------------------- | ---------- | --------------------------------------------------------- |
| `retry_interval(ms)`        | `unsigned` | Set reconnection interval (default `3000`)                |
| `use_independent_context()` | `bool`     | Run on a dedicated `io_context` thread managed by unilink |
| `auto_manage()`             | `bool`     | Auto-start immediately and stop on destruction            |

#### Instance Methods

| Method                 | Return         | Description                                                |
| ---------------------- | -------------- | ---------------------------------------------------------- |
| `send()`               | `void`         | Send data to device                                        |
| `send_line()`          | `void`         | Send data with trailing newline                            |
| `is_connected()`       | `bool`         | Check if port is open                                      |
| `start()`              | `void`         | Open serial port                                           |
| `stop()`               | `void`         | Close serial port                                          |
| `set_baud_rate()`      | `void`         | Adjust baud rate at runtime                                |
| `set_data_bits()`      | `void`         | Set data bits (5-8)                                        |
| `set_stop_bits()`      | `void`         | Set stop bits (1-2)                                        |
| `set_parity()`         | `void`         | Set parity (`none`, `even`, `odd`)                         |
| `set_flow_control()`   | `void`         | Set flow control (`none`, `software`, `hardware`)          |
| `set_retry_interval()` | `void`         | Adjust reconnection interval (`std::chrono::milliseconds`) |
| `build_config()`       | `SerialConfig` | Inspect current mapped serial config                       |

### Device Paths

**Linux:**

```cpp
"/dev/ttyUSB0"  // USB serial adapter
"/dev/ttyACM0"  // Arduino, CDC devices
"/dev/ttyS0"    // Built-in serial port
```

**Windows:**

```cpp
"COM3"
"COM4"
```

### Advanced Examples

#### Arduino Communication

```cpp
auto arduino = unilink::serial("/dev/ttyACM0", 9600)
    .on_connect([]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));  // Arduino reset delay
    })
    .on_data([](const std::string& data) {
        // Parse sensor data
        if (data.find("TEMP:") == 0) {
            float temp = std::stof(data.substr(5));
            std::cout << "Temperature: " << temp << "°C" << std::endl;
        }
    })
    .build();
```

#### GPS Module

```cpp
auto gps = unilink::serial("/dev/ttyUSB0", 9600)
    .on_data([](const std::string& nmea) {
        // Parse NMEA sentences
        if (nmea.find("$GPGGA") == 0) {
            // Parse GPS fix data
        }
    })
    .build();
```

---

## UDP Communication

Connectionless communication using UDP protocol.

### Basic Usage

#### UDP Receiver (Server)

```cpp
#include "unilink/unilink.hpp"

// Create a UDP socket bound to port 8080
auto receiver = unilink::udp(8080)
    .on_data([](const std::string& data) {
        std::cout << "Received: " << data << std::endl;
    })
    .build();

receiver->start();

// Keep running...
std::this_thread::sleep_for(std::chrono::seconds(60));
receiver->stop();
```

#### UDP Sender (Client)

```cpp
#include "unilink/unilink.hpp"

// Create a UDP socket and set remote destination
auto sender = unilink::udp(0)  // 0 = ephemeral port
    .set_remote("127.0.0.1", 8080)
    .build();

sender->start();
sender->send("Hello UDP!");
```

### API Reference

#### Constructor

```cpp
// Create UDP builder with local port binding
unilink::udp(uint16_t local_port)
```

#### Builder Methods

| Method                      | Parameters         | Description                                   |
| --------------------------- | ------------------ | --------------------------------------------- |
| `set_local_port(port)`      | `uint16_t`         | Bind to specific local port                   |
| `set_local_address(ip)`     | `string`           | Bind to specific local IP (default "0.0.0.0") |
| `set_remote(ip, port)`      | `string, uint16_t` | Set default destination for `send()`          |
| `use_independent_context()` | `bool`             | Run on dedicated IO thread                    |
| `auto_manage()`             | `bool`             | Auto-start/stop lifecycle                     |

#### Instance Methods

| Method           | Return | Description                            |
| ---------------- | ------ | -------------------------------------- |
| `send()`         | `void` | Send data to configured remote address |
| `start()`        | `void` | Start listening/sending                |
| `stop()`         | `void` | Close socket                           |
| `is_connected()` | `bool` | Check if socket is open and ready      |

### Advanced Examples

#### Echo Reply (Receiver)

```cpp
auto socket = unilink::udp(8080)
    .on_data([&](const std::string& data) {
        std::cout << "Received: " << data << std::endl;
        // Reply to the sender (automatically tracks last sender)
        // Note: Full symmetric echo requires manual address handling
        // which will be available in future updates.
    })
    .build();
```

---

## Error Handling

Centralized error handling system with callbacks and statistics.

### Setup Error Handler

```cpp
#include "unilink/diagnostics/error_handler.hpp"

using namespace unilink::common;

// Register global error callback
ErrorHandler::instance().register_callback([](const ErrorInfo& error) {
    std::cerr << "[" << error.component << "] "
              << error.message << std::endl;

    if (error.level == ErrorLevel::CRITICAL) {
        // Handle critical errors
        std::cerr << "CRITICAL ERROR! " << error.get_summary() << std::endl;
    }
});

// Set minimum error level
ErrorHandler::instance().set_min_error_level(ErrorLevel::WARNING);
```

### Error Levels

| Level      | Description      | Use Case              |
| ---------- | ---------------- | --------------------- |
| `INFO`     | Informational    | Status updates        |
| `WARNING`  | Potential issues | Non-critical problems |
| `ERROR`    | Serious errors   | Operation failures    |
| `CRITICAL` | Fatal errors     | System-wide issues    |

### Error Statistics

```cpp
auto stats = ErrorHandler::instance().get_error_stats();
std::cout << "Total errors: " << stats.total_errors << std::endl;
std::cout << "Critical: " << stats.critical_count << std::endl;
std::cout << "Errors: " << stats.error_count << std::endl;
```

### Error Categories

```cpp
namespace error_reporting = unilink::common::error_reporting;

// Connection errors
error_reporting::report_connection_error("tcp_client", "connect", ec, true);

// Communication errors
error_reporting::report_communication_error("serial", "read", "Timeout", false);

// Configuration errors
error_reporting::report_configuration_error("builder", "validate", "Invalid port");

// Memory errors
error_reporting::report_memory_error("buffer", "allocate", "Out of memory");
```

---

## Logging System

Flexible logging with multiple outputs and async processing.

### Basic Usage

```cpp
#include "unilink/diagnostics/logger.hpp"
#include "unilink/diagnostics/error_handler.hpp"

using namespace unilink::common;

// Get logger instance
auto& logger = Logger::instance();

// Configure logger
logger.set_level(LogLevel::DEBUG);
logger.set_console_output(true);
logger.set_file_output("app.log");

// Log messages
logger.debug("component", "operation", "Debug message");
logger.info("component", "operation", "Info message");
logger.warning("component", "operation", "Warning message");
logger.error("component", "operation", "Error message");
logger.critical("component", "operation", "Critical message");
```

### Log Levels

| Level      | Description         | Example                              |
| ---------- | ------------------- | ------------------------------------ |
| `DEBUG`    | Detailed debugging  | Variable values, flow control        |
| `INFO`     | General information | Status updates, milestones           |
| `WARNING`  | Potential issues    | Deprecated usage, recoverable errors |
| `ERROR`    | Error conditions    | Operation failures                   |
| `CRITICAL` | Critical failures   | System-wide issues                   |

### Async Logging

```cpp
// Enable async logging for better performance
AsyncLogConfig config;
config.batch_size = 100;                    // Process 100 logs at once
config.flush_interval = std::chrono::milliseconds(1000); // Flush every 1 second

logger.set_async_logging(true, config);
```

### Log Rotation

```cpp
#include "unilink/diagnostics/log_rotation.hpp"

LogRotation rotation;
rotation.set_max_file_size(10 * 1024 * 1024);  // 10 MB
rotation.set_max_backup_count(5);               // Keep 5 backups
rotation.rotate_if_needed("app.log");
```

---

## Configuration Management

_(Available when built with `UNILINK_ENABLE_CONFIG=ON`)_

### Load Configuration from File

```cpp
#include "unilink/config/config_manager.hpp"

using namespace unilink::config;

auto config = ConfigManager::load_from_file("config.json");

// Access configuration
auto host = config->get_tcp_client_config("main_client").host;
auto port = config->get_tcp_client_config("main_client").port;

// Create client from config
auto client = unilink::tcp_client(host, port)
    .retry_interval(config->get_tcp_client_config("main_client").retry_interval_ms)
    .build();
```

### Configuration File Format (JSON)

```json
{
  "tcp_clients": {
    "main_client": {
      "host": "192.168.1.100",
      "port": 8080,
      "retry_interval_ms": 3000
    }
  },
  "tcp_servers": {
    "main_server": {
      "port": 9000,
      "single_client_mode": false
    }
  },
  "serials": {
    "arduino": {
      "device": "/dev/ttyACM0",
      "baud_rate": 115200,
      "retry_interval_ms": 5000
    }
  }
}
```

---

## Advanced Features

### Memory Pool

Efficient memory allocation for high-performance scenarios.

```cpp
#include "unilink/memory/memory_pool.hpp"
#include "unilink/memory/safe_data_buffer.hpp"

using namespace unilink::common;

MemoryPool pool(1024, 100);  // 1KB blocks, 100 blocks

// Allocate from pool
auto buffer = pool.allocate();
// ... use buffer ...
pool.deallocate(buffer);
```

### Safe Data Buffer

Type-safe data buffer with bounds checking.

```cpp
#include "unilink/memory/safe_data_buffer.hpp"

using namespace unilink::common;

SafeDataBuffer buffer(1024);

// Safe operations
buffer.append("Hello");
buffer.append_bytes({0x01, 0x02, 0x03});

auto data = buffer.to_string();
auto bytes = buffer.to_vector();
```

### Thread-Safe State

Thread-safe state management with read-write locks.

```cpp
#include "unilink/concurrency/thread_safe_state.hpp"

using namespace unilink::common;

enum class State { Idle, Running, Stopped };

ThreadSafeState<State> state(State::Idle);

// Read state
auto current = state.get();

// Write state
state.set(State::Running);

// Atomic compare-and-swap
state.compare_exchange(State::Idle, State::Running);
```

---

## Best Practices

### 1. Always Handle Errors

```cpp
auto client = unilink::tcp_client("server.com", 8080)
    .on_error([](const std::string& error) {
        // Log error, notify user, etc.
    })
    .build();
```

### 2. Use Explicit Lifecycle Control

```cpp
// Always use explicit start/stop for clarity
auto client = unilink::tcp_client("127.0.0.1", 8080)
    .on_data(handler)
    .build();

client->start();  // Start when ready
// ... use the client ...
client->stop();   // Stop when done
```

### 3. Set Appropriate Retry Intervals

```cpp
// Fast retry for local connections
auto local_client = unilink::tcp_client("127.0.0.1", 8080)
    .retry_interval(1000)  // 1 second
    .build();

// Slower retry for remote connections
auto remote_client = unilink::tcp_client("remote.com", 8080)
    .retry_interval(10000)  // 10 seconds
    .build();
```

### 4. Enable Logging for Debugging

```cpp
unilink::common::Logger::instance().set_level(unilink::common::LogLevel::DEBUG);
unilink::common::Logger::instance().set_console_output(true);
```

### 5. Use Member Functions for OOP Design

```cpp
class MyApplication {
    std::shared_ptr<unilink::wrapper::TcpClient> client_;

    void on_data(const std::string& data) { /* ... */ }

    void start() {
        client_ = unilink::tcp_client("server.com", 8080)
            .on_data(this, &MyApplication::on_data)
            .build();
    }
};
```

---

## Performance Tips

### 1. Use Independent Context for Testing Only

```cpp
// Testing (isolated IO thread)
.use_independent_context(true)

// Production (shared IO thread - more efficient)
.use_independent_context(false)  // default
```

### 2. Enable Async Logging

```cpp
Logger::instance().enable_async(true);
```

### 3. Use Memory Pool for Frequent Allocations

```cpp
MemoryPool pool(buffer_size, pool_size);
```

### 4. Disable Unnecessary Features

```bash
# Build with minimal features
cmake -DUNILINK_ENABLE_CONFIG=OFF -DUNILINK_ENABLE_MEMORY_TRACKING=OFF
```
