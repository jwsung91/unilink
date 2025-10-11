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

## Your First TCP Client (30 seconds)

```cpp
#include <iostream>
#include <thread>
#include "unilink/unilink.hpp"

int main() {
    // Create a TCP client - it's that simple!
    auto client = unilink::tcp_client("127.0.0.1", 8080)
        .on_data([](const std::string& data) {
            std::cout << "Received: " << data << std::endl;
        })
        .build();
    
    // Start the connection
    client->start();
    
    // Wait a bit, then send a message
    std::this_thread::sleep_for(std::chrono::seconds(1));
    client->send("Hello, Server!");
    
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

## Your First TCP Server (30 seconds)

```cpp
#include <iostream>
#include <thread>
#include "unilink/unilink.hpp"

int main() {
    // Create a TCP server
    auto server = unilink::tcp_server(8080)
        .unlimited_clients()  // Required: set client limit
        .on_data([](size_t client_id, const std::string& data) {
            std::cout << "Client " << client_id << ": " << data << std::endl;
        })
        .build();
    
    // Start the server
    server->start();
    std::cout << "Server listening on port 8080..." << std::endl;
    
    // Keep running for 60 seconds
    std::this_thread::sleep_for(std::chrono::seconds(60));
    
    // Clean shutdown
    server->stop();
    return 0;
}
```

---

## Your First Serial Device (30 seconds)

```cpp
#include <iostream>
#include "unilink/unilink.hpp"

int main() {
    // Create serial connection
    auto serial = unilink::serial("/dev/ttyUSB0", 115200)
        .on_connect([]() {
            std::cout << "Serial port opened!" << std::endl;
        })
        .on_data([](const std::string& data) {
            std::cout << "Received: " << data << std::endl;
        })
        .build();
    
    serial->start();
    
    // Send data
    std::this_thread::sleep_for(std::chrono::seconds(1));
    serial->send("AT\r\n");
    
    // Keep running
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return 0;
}
```

---

## Common Patterns

### Pattern 1: Auto-Reconnection
```cpp
auto client = unilink::tcp_client("server.com", 8080)
    .retry_interval(3000)  // Retry every 3 seconds (default)
    .build();

client->start();  // Will automatically reconnect on disconnect
```

### Pattern 2: Error Handling
```cpp
auto server = unilink::tcp_server(8080)
    .on_error([](const std::string& error) {
        std::cerr << "Error: " << error << std::endl;
    })
    .enable_port_retry(true, 5, 1000)  // 5 retries, 1 sec interval
    .build();
```

### Pattern 3: Member Function Callbacks
```cpp
class MyApp {
    void on_data(const std::string& data) {
        // Handle data
    }
    
    void start() {
        auto client = unilink::tcp_client("127.0.0.1", 8080)
            .on_data(this, &MyApp::on_data)  // Member function!
            .build();
    }
};
```

### Pattern 4: Single vs Multi-Client Server
```cpp
// Single client only (reject others)
auto server = unilink::tcp_server(8080)
    .single_client()
    .build();

// Multiple clients (default)
auto server = unilink::tcp_server(8080)
    .multi_client()
    .build();
```

---

## Next Steps

1. **Read the API Guide**: `docs/API_GUIDE.md`
2. **Check Examples**: `examples/` directory
3. **Run Tests**: `cd build && ctest`
4. **View Full Docs**: `docs/html/index.html` (run `make docs` first)

---

## Troubleshooting

### Can't connect to server?
```cpp
// Enable logging to see what's happening
unilink::common::Logger::instance().set_level(unilink::common::LogLevel::DEBUG);
unilink::common::Logger::instance().set_console_output(true);
```

### Port already in use?
```cpp
auto server = unilink::tcp_server(8080)
    .enable_port_retry(true, 5, 1000)  // Try 5 times
    .build();
```

### Need independent IO thread?
```cpp
// For testing or isolation
auto client = unilink::tcp_client("127.0.0.1", 8080)
    .use_independent_context(true)
    .build();
```

---

## Support

- **GitHub Issues**: https://github.com/jwsung91/unilink/issues
- **Documentation**: `docs/` directory
- **Examples**: `examples/` directory

Happy coding! 🚀

