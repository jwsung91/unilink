# Unilink Quick Start Guide

Get started with unilink in 5 minutes!

## Installation

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt update && sudo apt install -y \
  build-essential cmake libboost-dev libboost-system-dev
```

### Build & Install

```bash
git clone https://github.com/jwsung91/unilink.git
cd unilink
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build
```

---

## Your First TCP Client

```cpp
#include <chrono>
#include <iostream>
#include <thread>
#include "unilink/unilink.hpp"

int main() {
    // Create a TCP client - it's that simple!
    auto client = unilink::tcp_client("127.0.0.1", 8080)
        .on_connect([](const unilink::ConnectionContext& ctx) {
            std::cout << "Connected!" << std::endl;
        })
        .on_data([](const unilink::MessageContext& ctx) {
            std::cout << "Received: " << ctx.data() << std::endl;
        })
        .build();

    bool started = client->start().get();

    // Send a message
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (started && client->connected()) {
        client->send("Hello, Server!");
    }

    // Keep running
    std::this_thread::sleep_for(std::chrono::seconds(5));
    client->stop();
    return 0;
}
```

**Compile:**

```bash
g++ -std=c++17 my_client.cc -lunilink -lboost_system -pthread -o my_client
./my_client
```

---

## Your First TCP Server

```cpp
#include <chrono>
#include <iostream>
#include <thread>
#include "unilink/unilink.hpp"

int main() {
    // Create a TCP server (defaults to unlimited clients)
    auto server = unilink::tcp_server(8080)
        .on_connect([](const unilink::ConnectionContext& ctx) {
            std::cout << "Client " << ctx.client_id() << " connected from " << ctx.client_info() << std::endl;
        })
        .on_data([](const unilink::MessageContext& ctx) {
            std::cout << "Client " << ctx.client_id() << ": " << ctx.data() << std::endl;
        })
        .build();

    if (!server->start().get()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    std::cout << "Server listening on port 8080..." << std::endl;

    // Keep running for 60 seconds
    std::this_thread::sleep_for(std::chrono::seconds(60));
    server->stop();
    return 0;
}
```

---

## Your First Serial Device

```cpp
#include <chrono>
#include <iostream>
#include <thread>
#include "unilink/unilink.hpp"

int main() {
    // Create serial connection
    auto serial = unilink::serial("/dev/ttyUSB0", 115200)
        .on_connect([](const unilink::ConnectionContext& ctx) {
            std::cout << "Serial port opened!" << std::endl;
        })
        .on_data([](const unilink::MessageContext& ctx) {
            std::cout << "Received: " << ctx.data() << std::endl;
        })
        .build();

    if (!serial->start().get()) {
        std::cerr << "Failed to open serial port" << std::endl;
        return 1;
    }

    // Send data
    std::this_thread::sleep_for(std::chrono::seconds(1));
    serial->send("AT\r\n");

    // Keep running
    std::this_thread::sleep_for(std::chrono::seconds(5));
    serial->stop();
    return 0;
}
```

---

## Common Patterns

### Pattern 1: Auto-Reconnection

```cpp
#include <chrono>
using namespace std::chrono_literals;

auto client = unilink::tcp_client("server.com", 8080)
    .retry_interval(3000ms)  // Retry every 3 seconds (default)
    .build();

client->start();  // Will automatically reconnect on disconnect
```

### Pattern 2: Error Handling

```cpp
auto server = unilink::tcp_server(8080)
    .on_error([](const unilink::ErrorContext& ctx) {
        std::cerr << "Error: " << ctx.message() << std::endl;
    })
    .port_retry(true, 5, 1000)  // 5 retries, 1 sec interval
    .build();
```

### Pattern 3: Single vs Multi-Client Server (optional)

```cpp
// Single client only (reject others)
auto server = unilink::tcp_server(8080)
    .single_client()
    .build();

// Multiple clients (set an explicit limit)
auto server = unilink::tcp_server(8080)
    .multi_client(8)  // allow up to 8 clients
    .build();

// Unlimited clients (default)
auto server = unilink::tcp_server(8080)
    .unlimited_clients()
    .build();
```

---

## Next Steps

1. **Read the API Guide**: `docs/user/api_guide.md`
2. **Check Examples**: `examples/` directory
3. **View Full Docs**: `docs/html/index.html` (run `cmake --build build --target docs` first)

---

## Troubleshooting

### Can't connect to server?

```cpp
// Enable logging to see what's happening
unilink::diagnostics::Logger::instance().set_level(unilink::diagnostics::LogLevel::DEBUG);
unilink::diagnostics::Logger::instance().set_console_output(true);
```

### Port already in use?

```cpp
auto server = unilink::tcp_server(8080)
    .unlimited_clients()
    .port_retry(true, 5, 1000)  // Try 5 times
    .build();
```

### Need independent IO thread?

```cpp
// For testing or isolation
auto client = unilink::tcp_client("127.0.0.1", 8080)
    .independent_context(true)
    .build();
```

---

## Support

- **GitHub Issues**: https://github.com/jwsung91/unilink/issues
- **Documentation**: `docs/` directory
- **Examples**: `examples/` directory

Happy coding! 🚀
