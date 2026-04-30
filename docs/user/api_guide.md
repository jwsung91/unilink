# Unilink API Guide {#user_api_guide}

Comprehensive API reference for the unilink library.

---

## Table of Contents

1. [Builder API](#builder-api)
2. [TCP Client](#tcp-client)
3. [TCP Server](#tcp-server)
4. [Serial Communication](#serial-communication)
5. [UDP Communication](#udp-communication)
6. [UDS Communication](#uds-communication)
7. [Error Handling](#error-handling)
8. [Logging System](#logging-system)
9. [Configuration Management](#configuration-management)
10. [Backpressure Strategy](#backpressure-strategy)
11. [Security](#security)

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

| Method                           | Description                                                       | Default  |
| -------------------------------- | ----------------------------------------------------------------- | -------- |
| `.on_data(callback)`             | Handle incoming data (`const MessageContext&`)                    | None     |
| `.on_connect(callback)`          | Handle connection events (`const ConnectionContext&`)             | None     |
| `.on_disconnect(callback)`       | Handle disconnection (`const ConnectionContext&`)                 | None     |
| `.on_error(callback)`            | Handle errors (`const ErrorContext&`)                             | None     |
| `.on_backpressure(callback)`     | Handle queue threshold events (`void(size_t bytes)`)              | None     |
| `.backpressure_threshold(bytes)` | Set queue limit for strategy or flow control                      | 512 KB   |
| `.backpressure_strategy(enum)`   | Set behavior when threshold is reached (`Reliable`, `BestEffort`)  | `Reliable`|
| `.auto_start(bool)`             | Auto-start/stop the wrapper (starts immediately when `true`)      | `false`  |
| `.independent_context(bool)`     | Create and run a dedicated `io_context` thread managed by unilink | `false`  |
| `.use_line_framer(...)`          | Split incoming bytes into delimiter-based messages                | Disabled |
| `.use_packet_framer(...)`        | Split incoming bytes into packet-based messages                   | Disabled |
| `.on_message(callback)`          | Handle framed messages (`const MessageContext&`)                  | None     |
| `.build()`                       | **Required**: Build the wrapper instance                          | -        |

**`MessageContext` Data Access**

Inside `on_data` and `on_message` callbacks, `ctx.data()` returns a `std::string_view` that is only valid for the duration of the callback. Do not store or capture it beyond the callback scope.

To take ownership of the data, use:
- `ctx.data_as_string()` — returns `std::string` (copy)
- `ctx.data_as_vector()` — returns `std::vector<uint8_t>` (copy)

```cpp
.on_data([](const unilink::MessageContext& ctx) {
    // OK: use within the callback
    std::cout << ctx.data() << std::endl;

    // OK: take ownership if you need to store it
    std::string owned = ctx.data_as_string();
    queue.push(std::move(owned));

    // BAD: do not store the string_view beyond this callback
    // captured_view = ctx.data();  // dangling reference!
})
```

**Builder-Specific Options**

- `TcpClientBuilder` / `SerialBuilder`: `.retry_interval(ms)` (default `3000ms`)
- `TcpServerBuilder`: `.port_retry(enable, max_retries, retry_interval_ms)`
- `TcpServerBuilder`: `.single_client()`, `.multi_client(max>=2)`, `.unlimited_clients()` (Defaults to `unlimited_clients()` if not specified)
- TCP server callbacks use the same Context-based signatures. Use `ctx.client_id()` and `ctx.client_info()` to distinguish clients.

### Framed Message Handling

Use `.on_message()` together with a framer when you want callback flow to operate on complete framed messages instead of raw byte chunks.

**Benefits:**

- **Performance**: Avoids `std::string` allocation overhead.
- **Safety**: Bounds-checked access preventing buffer overflows.

**Example:**

```cpp
.use_line_framer("\n")
.on_message([](const unilink::MessageContext& ctx) {
    std::cout << "Framed message: " << ctx.data() << std::endl;
})
```

**Lifecycle Methods:**
| Method | Description |
| ------------------------------------ | ---------------------------------------------------------------------- |
| `->start()` | Start the connection (returns `std::future<bool>`) |
| `->start_sync()` | Start the connection and block until established (returns `bool`) |
| `->stop()` | Stop the connection |
| `TcpClient` / `Serial` / `UdpClient` / `UdsClient`: `->send(data)` / `->send_line(text)` | Send data to the configured peer |
| `TcpClient` / `Serial` / `UdpClient` / `UdsClient`: `->connected()` | Check channel state |

| `TcpServer` / `UdsServer`: `->broadcast(data)` / `->send_to(client_id, data)` | Send data to one or more connected clients |
| `TcpServer` / `UdsServer`: `->listening()` | Check if the server socket is bound and listening |

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
- **`independent_context(true)`**: Builder creates its own `io_context` and runs it on an internal thread; cleanup is automatic.
- **External `io_context`**: If you manually pass a custom `io_context` to wrapper constructors, unilink will _not_ run/stop it unless you call `manage_external_context(true)` on the wrapper. In that case, callbacks should be registered before enabling `auto_start(true)` (it starts immediately).

---

## TCP Client

Connect to remote TCP servers with automatic reconnection.

### Basic Usage

```cpp
#include "unilink/unilink.hpp"

auto client = unilink::tcp_client("192.168.1.100", 8080)
    .on_connect([](const unilink::ConnectionContext& ctx) {
        std::cout << "Connected!" << std::endl;
    })
    .on_data([](const unilink::MessageContext& ctx) {
        std::cout << "Received: " << ctx.data() << std::endl;
    })
    .on_disconnect([](const unilink::ConnectionContext& ctx) {
        std::cout << "Disconnected" << std::endl;
    })
    .on_error([](const unilink::ErrorContext& ctx) {
        std::cerr << "Error: " << ctx.message() << std::endl;
    })
    .retry_interval(3000ms)  // Optional: Retry every 3 seconds (default)
    .build();

// Start connection
bool connected = client->start().get();

// Send data
if (connected && client->connected()) {
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
| `max_retries(count)`        | `int`      | Set maximum reconnect attempts (`-1` for unlimited)        |
| `connection_timeout(ms)`    | `unsigned` | Set connection timeout in milliseconds                     |
| `independent_context()`     | `bool`     | Use separate IO thread (for testing)                       |
| `auto_start()`             | `bool`     | Auto-start immediately and stop on destruction             |

#### Instance Methods

| Method                     | Return              | Description                                                           |
| -------------------------- | ------------------- | --------------------------------------------------------------------- |
| `send()`                   | `bool`              | Enqueue data for sending; `false` if not connected or queue rejected |
| `send_line()`              | `bool`              | Enqueue data plus trailing newline                                    |
| `connected()`             | `bool`              | Check connection status                                               |
| `start()`                  | `std::future<bool>` | Start connection attempt                                              |
| `stop()`                   | `void`              | Stop and disconnect                                                   |
| `retry_interval()`     | `TcpClient&`        | Adjust reconnection interval at runtime (`std::chrono::milliseconds`) |
| `max_retries()`        | `TcpClient&`        | Set max reconnect attempts (`-1` for unlimited)                       |
| `connection_timeout()` | `TcpClient&`        | Set connection timeout (`std::chrono::milliseconds`)                  |

### Advanced Examples

#### With Member Functions

```cpp
class MyClient {
    std::unique_ptr<unilink::TcpClient> client_;

public:
    void on_data(const unilink::MessageContext& ctx) {
        // Handle data: ctx.data()
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
    .on_data([device_id](const unilink::MessageContext& ctx) {
        std::cout << "[" << device_id << "] " << ctx.data() << std::endl;
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
    .unlimited_clients()
    .on_connect([](const unilink::ConnectionContext& ctx) {
        std::cout << "Client " << ctx.client_id() << " connected from " << ctx.client_info() << std::endl;
    })
    .on_data([](const unilink::MessageContext& ctx) {
        std::cout << "Client " << ctx.client_id() << ": " << ctx.data() << std::endl;
    })
    .on_disconnect([](const unilink::ConnectionContext& ctx) {
        std::cout << "Client " << ctx.client_id() << " disconnected" << std::endl;
    })
    .build();

// Start server
bool listening = server->start().get();

// Send to specific client
if (listening) {
    server->send_to(1, "Hello, Client 1!");
}

// Send to all clients
if (listening) {
    server->broadcast("Broadcast message");
}

// Check if listening
if (listening && server->listening()) {
    std::cout << "Server is listening" << std::endl;
}

// Clean shutdown
server->stop();
```

**Note:** If none of `.single_client()`, `.multi_client(max)`, or `.unlimited_clients()` is called before `build()`, the server defaults to unlimited clients.

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
| `port_retry()`       | `bool, retries, interval_ms` | Retry if port is in use                                              |
| `independent_context()`     | `bool`                       | Run on a dedicated `io_context` thread managed by unilink            |
| `auto_start()`             | `bool`                       | Auto-start immediately and stop on destruction                       |

Multi-client callbacks use the standard `ConnectionContext` and `MessageContext` which contain `client_id()` and `client_info()` accessors.

#### Instance Methods

| Method                    | Return                | Description                            |
| ------------------------- | --------------------- | -------------------------------------- |
| `broadcast()`             | `bool`                | Send to all connected clients          |
| `send_to()`               | `bool`                | Send to a specific client              |
| `client_count()`      | `size_t`              | Number of connected clients            |
| `connected_clients()` | `std::vector<size_t>` | List of connected client IDs           |
| `on_connect()`            | `ServerInterface&`    | Register runtime connect callback      |
| `on_disconnect()`         | `ServerInterface&`    | Register runtime disconnect callback   |
| `on_data()`               | `ServerInterface&`    | Register runtime message callback      |
| `on_error()`              | `ServerInterface&`    | Register runtime error callback        |
| `listening()`             | `bool`                | Check if server is listening           |
| `start()`                 | `std::future<bool>`   | Start accepting connections            |
| `stop()`                  | `void`                | Stop server and disconnect all clients |

### Advanced Examples

#### Single Client Mode

```cpp
auto server = unilink::tcp_server(8080)
    .single_client()  // Only one client allowed
    .on_connect([](const unilink::ConnectionContext& ctx) {
        std::cout << "Client connected: " << ctx.client_info() << std::endl;
    })
    .build();
```

#### Port Retry

```cpp
auto server = unilink::tcp_server(8080)
    .single_client()
    .port_retry(true, 5, 1000)  // 5 retries, 1 second each
    .on_error([](const unilink::ErrorContext& ctx) {
        std::cerr << "Server error: " << ctx.message() << std::endl;
    })
    .build();
```

#### Echo Server Pattern

```cpp
auto server = unilink::tcp_server(8080)
    .unlimited_clients()
    .build();

server->on_data([&server](const unilink::MessageContext& ctx) {
    server->send_to(ctx.client_id(), "Echo: " + std::string(ctx.data()));
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
    .on_connect([](const unilink::ConnectionContext& ctx) {
        std::cout << "Serial port opened" << std::endl;
    })
    .on_data([](const unilink::MessageContext& ctx) {
        std::cout << "Received: " << ctx.data() << std::endl;
    })
    .build();

// Start serial communication
bool opened = serial->start().get();

// Send AT command
if (opened) {
    serial->send("AT\r\n");
}

// Send binary payload through string-compatible storage
std::string binary_payload("\x01\x02\x03", 3);
if (opened) {
    serial->send(binary_payload);
}
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
| `data_bits(bits)`           | `int`      | Set serial data bits before `build()`                     |
| `stop_bits(bits)`           | `int`      | Set serial stop bits before `build()`                     |
| `parity(mode)`              | `string`   | Set serial parity before `build()`                        |
| `flow_control(mode)`        | `string`   | Set flow control before `build()`                         |
| `retry_interval(ms)`        | `unsigned` | Set reconnection interval (default `3000`)                |
| `independent_context()`     | `bool`     | Run on a dedicated `io_context` thread managed by unilink |
| `auto_start()`             | `bool`     | Auto-start immediately and stop on destruction            |

#### Instance Methods

| Method                 | Return              | Description                                                |
| ---------------------- | ------------------- | ---------------------------------------------------------- |
| `send()`               | `bool`              | Enqueue data for transmission                              |
| `send_line()`          | `bool`              | Enqueue data with trailing newline                         |
| `connected()`          | `bool`              | Check if port is open                                      |
| `start()`              | `std::future<bool>` | Open serial port                                           |
| `stop()`               | `void`              | Close serial port                                          |
| `baud_rate()`      | `Serial&`           | Adjust baud rate at runtime                                |
| `data_bits()`      | `Serial&`           | Set data bits (5-8)                                        |
| `stop_bits()`      | `Serial&`           | Set stop bits (1-2)                                        |
| `parity()`         | `Serial&`           | Set parity (`none`, `even`, `odd`)                         |
| `flow_control()`   | `Serial&`           | Set flow control (`none`, `software`, `hardware`)          |
| `retry_interval()` | `Serial&`           | Adjust reconnection interval (`std::chrono::milliseconds`) |

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
    .on_connect([](const unilink::ConnectionContext& ctx) {
        std::this_thread::sleep_for(std::chrono::seconds(2));  // Arduino reset delay
    })
    .on_data([](const unilink::MessageContext& ctx) {
        // Parse sensor data
        std::string_view data = ctx.data();
        if (data.find("TEMP:") == 0) {
            float temp = std::stof(std::string(data.substr(5)));
            std::cout << "Temperature: " << temp << "°C" << std::endl;
        }
    })
    .build();
```

#### GPS Module

```cpp
auto gps = unilink::serial("/dev/ttyUSB0", 9600)
    .on_data([](const unilink::MessageContext& ctx) {
        // Parse NMEA sentences
        if (ctx.data().find("$GPGGA") == 0) {
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
auto receiver = unilink::udp_client(8080)
    .on_data([](const unilink::MessageContext& ctx) {
        std::cout << "Received: " << ctx.data() << std::endl;
    })
    .build();

bool receiver_started = receiver->start().get();

// Keep running...
if (receiver_started) {
    std::this_thread::sleep_for(std::chrono::seconds(60));
    receiver->stop();
}
```

#### UDP Sender (Client)

```cpp
#include "unilink/unilink.hpp"

// Create a UDP socket and set remote destination
auto sender = unilink::udp_client(0)  // 0 = ephemeral port
    .remote("127.0.0.1", 8080)
    .build();

bool sender_started = sender->start().get();
if (sender_started) {
    sender->send("Hello UDP!");
}
```

### API Reference

#### Constructors

```cpp
// UDP client: send and/or receive with a configured remote peer
unilink::udp_client(uint16_t local_port)

// UDP server: receive-only listener with virtual sessions per sender
unilink::udp_server(uint16_t local_port)
```

#### Builder Methods (UdpClient)

| Method                      | Parameters         | Description                          |
| --------------------------- | ------------------ | ------------------------------------ |
| `local_port(port)`          | `uint16_t`         | Bind to a specific local port        |
| `remote(ip, port)`          | `string, uint16_t` | Set default destination for `send()` |
| `broadcast(enable)`         | `bool`             | Enable broadcast sends               |
| `reuse_address(enable)`     | `bool`             | Enable SO_REUSEADDR on the socket    |
| `independent_context()`     | `bool`             | Run on dedicated IO thread           |
| `auto_start()`              | `bool`             | Auto-start/stop lifecycle            |

#### Builder Methods (UdpServer)

| Method                      | Parameters | Description                       |
| --------------------------- | ---------- | --------------------------------- |
| `local_port(port)`          | `uint16_t` | Bind to a specific local port     |
| `broadcast(enable)`         | `bool`     | Enable broadcast receives         |
| `reuse_address(enable)`     | `bool`     | Enable SO_REUSEADDR on the socket |
| `independent_context()`     | `bool`     | Run on dedicated IO thread        |
| `auto_start()`              | `bool`     | Auto-start/stop lifecycle         |

#### Instance Methods (UdpClient)

| Method           | Return              | Description                               |
| ---------------- | ------------------- | ----------------------------------------- |
| `send()`         | `bool`              | Enqueue data to configured remote address |
| `start()`        | `std::future<bool>` | Start listening/sending                   |
| `stop()`         | `void`              | Close socket                              |
| `connected()`    | `bool`              | Check if socket is open and ready         |

### Advanced Examples

#### Echo Reply (Receiver)

```cpp
auto socket = unilink::udp_client(8080)
    .on_data([&](const unilink::MessageContext& ctx) {
        std::cout << "Received: " << ctx.data() << std::endl;
        // Reply to the sender (automatically tracks last sender)
    })
    .build();
```

#### UDP Server (Receive-only listener)

```cpp
auto server = unilink::udp_server(8080)
    .on_data([](const unilink::MessageContext& ctx) {
        std::cout << "Received: " << ctx.data() << std::endl;
    })
    .build();

server->start().get();
```

---

## UDS Communication

High-performance local inter-process communication (IPC) using Unix Domain Sockets.

### Basic Usage

#### UDS Server

```cpp
#include "unilink/unilink.hpp"

auto server = unilink::uds_server("/tmp/my_service.sock")
    .unlimited_clients()
    .on_connect([](const unilink::ConnectionContext& ctx) {
        std::cout << "Client connected!" << std::endl;
    })
    .on_data([](const unilink::MessageContext& ctx) {
        std::cout << "Received: " << ctx.data() << std::endl;
    })
    .build();

bool listening = server->start().get();
if (!listening) {
    std::cerr << "Failed to start UDS server" << std::endl;
}
```

#### UDS Client

```cpp
#include "unilink/unilink.hpp"

auto client = unilink::uds_client("/tmp/my_service.sock")
    .on_connect([](const unilink::ConnectionContext& ctx) {
        std::cout << "Connected to UDS server!" << std::endl;
    })
    .build();

bool connected = client->start().get();
if (connected) {
    client->send("Hello IPC!");
}
```

### API Reference

#### Constructors

```cpp
unilink::uds_server(const std::string& socket_path)
unilink::uds_client(const std::string& socket_path)
```

#### Builder Methods (UDS Server)

| Method                      | Parameters | Description                          |
| --------------------------- | ---------- | ------------------------------------ |
| `max_clients(max)`          | `size_t`   | Allow up to `max` concurrent clients |
| `unlimited_clients()`       | None       | Allow unlimited clients              |
| `independent_context()`     | `bool`     | Run on dedicated IO thread           |
| `auto_start()`             | `bool`     | Auto-start/stop lifecycle            |

#### Builder Methods (UDS Client)

| Method                      | Parameters | Description                                |
| --------------------------- | ---------- | ------------------------------------------ |
| `retry_interval(ms)`        | `unsigned` | Set reconnection interval (default `3000`) |
| `max_retries(count)`        | `int`      | Set maximum reconnect attempts             |
| `connection_timeout(ms)`    | `unsigned` | Set connection timeout in milliseconds     |
| `independent_context()`     | `bool`     | Run on a dedicated `io_context` thread     |
| `auto_start()`             | `bool`     | Auto-start/stop lifecycle                  |

#### Instance Methods (UDS Client)

| Method            | Return              | Description                           |
| ----------------- | ------------------- | ------------------------------------- |
| `start()`         | `std::future<bool>` | Start communication                   |
| `stop()`          | `void`              | Stop communication                    |
| `connected()`     | `bool`              | Check if the client channel is active |
| `send(data)`      | `bool`              | Enqueue data to the server            |
| `send_line(text)` | `bool`              | Enqueue text with a trailing newline  |

#### Instance Methods (UDS Server)

| Method                     | Return                | Description                      |
| -------------------------- | --------------------- | -------------------------------- |
| `start()`                  | `std::future<bool>`   | Start accepting connections      |
| `stop()`                   | `void`                | Stop the server                  |
| `listening()`              | `bool`                | Check if the socket is listening |
| `broadcast(data)`          | `bool`                | Send to all connected clients    |
| `send_to(client_id, data)` | `bool`                | Send to a specific client        |
| `client_count()`       | `size_t`              | Number of connected clients      |
| `connected_clients()`  | `std::vector<size_t>` | List of connected client IDs     |

### Notes on UDS

- **Platform Support**: Unix Domain Sockets are natively supported on Linux, macOS, and recent versions of Windows 10/11.
- **Path Length**: Socket paths are typically limited to ~108 characters (standard `sockaddr_un` limit).
- **Cleanup**: Unilink automatically removes the socket file when the server starts and stops to ensure clean initialization.

---

## Error Handling

Centralized error handling system with callbacks and statistics.

### Setup Error Handler

```cpp
#include "unilink/diagnostics/error_handler.hpp"

using namespace unilink::diagnostics;

// Register global error callback
ErrorHandler::instance().register_callback([](const ErrorInfo& error) {
    std::cerr << "[" << error.component << "] "
              << error.message << std::endl;

    if (error.level == ErrorLevel::CRITICAL) {
        // Handle critical errors
        std::cerr << "CRITICAL ERROR! " << error.summary() << std::endl;
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

---

## Logging System

Flexible logging with multiple outputs and async processing.

### Basic Usage

```cpp
#include "unilink/diagnostics/logger.hpp"
#include "unilink/diagnostics/error_handler.hpp"

auto& logger = unilink::diagnostics::Logger::instance();

// Get logger instance
// Configure logger
logger.set_level(unilink::diagnostics::LogLevel::DEBUG);
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
unilink::diagnostics::AsyncLogConfig config;
config.batch_size = 100;                    // Process 100 logs at once
config.flush_interval = std::chrono::milliseconds(1000); // Flush every 1 second

logger.set_async_logging(true, config);
```

---

## Configuration Management

_(Available when built with `UNILINK_ENABLE_CONFIG=ON`)_

### Load Configuration from File

```cpp
#include <any>
#include "unilink/config/config_factory.hpp"

auto config = unilink::config::ConfigFactory::create_with_defaults();
config->load_from_file("unilink.conf");

auto host = std::any_cast<std::string>(config->get("tcp.client.host"));
auto port = static_cast<uint16_t>(std::any_cast<int>(config->get("tcp.client.port")));
auto retry_interval_ms = static_cast<unsigned>(
    std::any_cast<int>(config->get("tcp.client.retry_interval_ms"))
);

// Create client from config
auto client = unilink::tcp_client(host, port)
    .retry_interval(retry_interval_ms)
    .build();
```

### Configuration File Format

The current configuration manager reads simple `key=value` files.

```ini
# unilink.conf
tcp.client.host=192.168.1.100
tcp.client.port=8080
tcp.client.retry_interval_ms=3000

tcp.server.port=9000
tcp.server.max_connections=100

serial.port=/dev/ttyACM0
serial.baud_rate=115200
serial.retry_interval_ms=5000
```

Common preset keys are populated by `unilink::config::ConfigPresets` through `ConfigFactory::create_with_defaults()`.

---

## Best Practices

### 1. Always Handle Errors

```cpp
auto client = unilink::tcp_client("server.com", 8080)
    .on_error([](const unilink::ErrorContext& ctx) {
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
    .retry_interval(1000ms)  // 1 second
    .build();

// Slower retry for remote connections
auto remote_client = unilink::tcp_client("remote.com", 8080)
    .retry_interval(10000ms)  // 10 seconds
    .build();
```

### 4. Enable Logging for Debugging

```cpp
unilink::diagnostics::Logger::instance().set_level(unilink::diagnostics::LogLevel::DEBUG);
unilink::diagnostics::Logger::instance().set_console_output(true);
```

### 5. Use Member Functions for OOP Design

```cpp
class MyApplication {
    std::unique_ptr<unilink::TcpClient> client_;

    void on_data(const unilink::MessageContext& ctx) { /* ... */ }

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
.independent_context(true)

// Production (shared IO thread - more efficient)
.independent_context(false)  // default
```

### 2. Enable Async Logging

```cpp
unilink::diagnostics::AsyncLogConfig config;
config.batch_size = 100;
unilink::diagnostics::Logger::instance().set_async_logging(true, config);
```

---

## Backpressure Strategy

When a sender produces data faster than the network can deliver it, messages accumulate in the send queue. The `BackpressureStrategy` enum controls what happens when the queue approaches its threshold — trading off **completeness** against **freshness**.

### Strategies

| Strategy     | Behaviour at threshold                     | Use when…                                           |
| ------------ | ------------------------------------------ | --------------------------------------------------- |
| `Reliable`    | Keep queuing until the hard cap is reached | Every message must arrive — files, commands, logs   |
| `BestEffort` | Drop the entire queue; keep sending fresh data | Latency matters more than completeness — sensor streams, robot state, video telemetry |

`Reliable` is the default for all transports. It is the safe choice: no data is silently discarded.

`BestEffort` is inspired by DDS HISTORY QoS. When the queue exceeds the backpressure threshold, all queued (stale) data is discarded and only the newest write is enqueued. The `on_backpressure` callback fires each time the queue is flushed so callers can observe drops.

### When to use each

**Use `Reliable` (default) when:**
- Reliability is critical: file transfers, command sequences, logs, financial data
- Your consumer is expected to keep up, or your backpressure threshold is generously sized
- A silent drop is a bug, not an acceptable trade-off

**Use `BestEffort` when:**
- You publish high-frequency sensor readings and the consumer only cares about the current state
- You are streaming robot joint angles, camera frames, or GNSS positions over a slow link
- A stale value is worse than a dropped value

### C++ Usage

Set the strategy via the transport config before starting:

```cpp
#include "unilink/unilink.hpp"
#include "unilink/base/constants.hpp"

using unilink::base::constants::BackpressureStrategy;

// TCP client — real-time sensor stream
config::TcpClientConfig cfg;
cfg.host = "192.168.1.10";
cfg.port = 8080;
cfg.backpressure_threshold = 512 * 1024;            // 512 KB flush threshold
cfg.backpressure_strategy  = BackpressureStrategy::BestEffort;

auto client = TcpClient::create(cfg, ioc);
client->on_backpressure([](size_t /* dropped_bytes */) {
    // queue was flushed — stale data discarded
});
```

Or via the wrapper builder (fluent API):

```cpp
#include "unilink/unilink.hpp"

auto client = unilink::tcp_client("192.168.1.10", 8080)
    .backpressure_threshold(512 * 1024)
    .backpressure_strategy(unilink::base::constants::BackpressureStrategy::BestEffort)
    .on_backpressure([](size_t) { /* queue flushed */ })
    .build();
```

The strategy can also be changed at runtime on a running transport:

```cpp
// Switch strategy dynamically (e.g. entering a high-rate burst mode)
client->set_backpressure_strategy(BackpressureStrategy::BestEffort);
```

### Python Usage

```python
import unilink_py as unilink

client = unilink.TcpClient("192.168.1.10", 8080)
client.backpressure_threshold = 512 * 1024
client.backpressure_strategy  = unilink.BackpressureStrategy.BestEffort
client.start()
```

### Thresholds

Unilink uses **Dynamic Defaults** for the backpressure threshold based on the selected strategy. If you do not explicitly set a threshold, the following values are used:

| Strategy | Default Threshold | Typical Use Case |
| :--- | :--- | :--- |
| `Reliable` | **4 MiB** | Commands, logs, file transfers |
| `BestEffort` | **512 KiB** | LiDAR, Camera, Real-time state |

A hard cap (`bp_limit_`) is automatically calculated as `max(threshold * 4, 4MB)` to prevent unbounded memory use while ensuring enough room for large individual messages.

| Parameter | Default | Notes |
| :--- | :--- | :--- |
| `backpressure_threshold` | *Dynamic* | 4MB for Reliable, 512KB for BestEffort |
| Hard cap (`bp_limit_`) | ≥ 4 MiB | Per-message reject limit |

---

## Security

### Validate All Input

Always validate data received from the network before processing it.

```cpp
void handle_message(const std::string& msg) {
    if (msg.empty() || msg.size() > MAX_MESSAGE_SIZE) {
        log_warning("Rejected invalid message");
        return;
    }
    if (!is_valid_format(msg)) {
        log_warning("Invalid message format");
        return;
    }
    process_message(msg);
}
```

### Rate Limiting

Protect your server from abuse by limiting requests per client.

```cpp
class RateLimiter {
    std::map<size_t, std::deque<std::chrono::steady_clock::time_point>> request_times_;
    const size_t max_requests_per_second_{10};

    bool is_allowed(size_t client_id) {
        auto now = std::chrono::steady_clock::now();
        auto& times = request_times_[client_id];
        while (!times.empty() && now - times.front() > std::chrono::seconds(1))
            times.pop_front();
        if (times.size() >= max_requests_per_second_) return false;
        times.push_back(now);
        return true;
    }
};

server->on_data([this, &limiter](const unilink::MessageContext& ctx) {
    if (!limiter.is_allowed(ctx.client_id())) return;
    process_data(ctx.client_id(), std::string(ctx.data()));
});
```

### Connection Limits

Use `.multi_client(N)` to cap simultaneous connections, or enforce limits manually:

```cpp
// Built-in limit via builder
auto server = unilink::tcp_server(8080)
    .multi_client(100)  // max 100 clients
    .build();
```

