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

| Method | Description | Default |
|--------|-------------|---------|
| `.auto_start(bool)` | Start automatically on build | `false` |
| `.auto_manage(bool)` | Auto resource management | `true` |
| `.on_data(callback)` | Handle incoming data | None |
| `.on_connect(callback)` | Handle connection events | None |
| `.on_disconnect(callback)` | Handle disconnection | None |
| `.on_error(callback)` | Handle errors | None |
| `.retry_interval(ms)` | Reconnection interval | `5000` |
| `.use_independent_context(bool)` | Use separate IO thread | `false` |

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
    .retry_interval(3000)  // Retry every 3 seconds
    .auto_start(true)
    .build();

// Send data
if (client->is_connected()) {
    client->send("Hello, Server!");
}

// Stop client
client->stop();
```

### API Reference

#### Constructor
```cpp
unilink::tcp_client(const std::string& host, uint16_t port)
```

#### Builder Methods
| Method | Parameters | Description |
|--------|------------|-------------|
| `retry_interval()` | `unsigned ms` | Set reconnection interval in milliseconds |
| `use_independent_context()` | `bool` | Use separate IO thread (for testing) |

#### Instance Methods
| Method | Return | Description |
|--------|--------|-------------|
| `send()` | `void` | Send data to server |
| `is_connected()` | `bool` | Check connection status |
| `start()` | `void` | Start connection attempt |
| `stop()` | `void` | Stop and disconnect |

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
    .auto_start(true)
    .build();

// Send to specific client
server->send_to_client(1, "Hello, Client 1!");

// Send to all clients
server->send("Broadcast message");

// Check if listening
if (server->is_listening()) {
    std::cout << "Server is listening" << std::endl;
}
```

### API Reference

#### Constructor
```cpp
unilink::tcp_server(uint16_t port)
```

#### Builder Methods
| Method | Parameters | Description |
|--------|------------|-------------|
| `single_client()` | None | Accept only one client at a time |
| `multi_client()` | None | Accept multiple clients (default) |
| `enable_port_retry()` | `bool, retries, interval_ms` | Retry if port is in use |

#### Instance Methods
| Method | Return | Description |
|--------|--------|-------------|
| `send()` | `void` | Send to all clients |
| `send_to_client()` | `void` | Send to specific client |
| `is_listening()` | `bool` | Check if server is listening |
| `start()` | `void` | Start accepting connections |
| `stop()` | `void` | Stop server and disconnect all |

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
    .on_data([](size_t client_id, const std::string& data) {
        // Echo back to sender
        server->send_to_client(client_id, "Echo: " + data);
    })
    .build();
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
    .retry_interval(3000)
    .auto_start(true)
    .build();

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
| Method | Parameters | Description |
|--------|------------|-------------|
| `retry_interval()` | `unsigned ms` | Set reconnection interval |
| `use_independent_context()` | `bool` | Use separate IO thread |

#### Instance Methods
| Method | Return | Description |
|--------|--------|-------------|
| `send()` | `void` | Send data to device |
| `is_connected()` | `bool` | Check if port is open |
| `start()` | `void` | Open serial port |
| `stop()` | `void` | Close serial port |

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

## Error Handling

Centralized error handling system with callbacks and statistics.

### Setup Error Handler

```cpp
#include "unilink/common/error_handler.hpp"

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

| Level | Description | Use Case |
|-------|-------------|----------|
| `INFO` | Informational | Status updates |
| `WARNING` | Potential issues | Non-critical problems |
| `ERROR` | Serious errors | Operation failures |
| `CRITICAL` | Fatal errors | System-wide issues |

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
#include "unilink/common/logger.hpp"

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

| Level | Description | Example |
|-------|-------------|---------|
| `DEBUG` | Detailed debugging | Variable values, flow control |
| `INFO` | General information | Status updates, milestones |
| `WARNING` | Potential issues | Deprecated usage, recoverable errors |
| `ERROR` | Error conditions | Operation failures |
| `CRITICAL` | Critical failures | System-wide issues |

### Async Logging

```cpp
// Enable async logging for better performance
logger.enable_async(true);
logger.set_batch_size(100);     // Process 100 logs at once
logger.set_flush_interval(1000); // Flush every 1 second
```

### Log Rotation

```cpp
#include "unilink/common/log_rotation.hpp"

LogRotation rotation;
rotation.set_max_file_size(10 * 1024 * 1024);  // 10 MB
rotation.set_max_backup_count(5);               // Keep 5 backups
rotation.rotate_if_needed("app.log");
```

---

## Configuration Management

*(Available when built with `UNILINK_ENABLE_CONFIG=ON`)*

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
#include "unilink/common/memory_pool.hpp"

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
#include "unilink/common/safe_data_buffer.hpp"

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
#include "unilink/common/thread_safe_state.hpp"

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

### 2. Use Auto-Start for Simple Cases
```cpp
// Starts automatically after build()
auto client = unilink::tcp_client("127.0.0.1", 8080)
    .auto_start(true)
    .build();
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

---

## Support & Resources

- **Examples**: `examples/` directory
- **Tests**: `test/` directory
- **Full Documentation**: Run `make docs` and open `docs/html/index.html`
- **GitHub**: https://github.com/jwsung91/unilink

---

**Version**: 1.0  
**Last Updated**: 2025-10-11

