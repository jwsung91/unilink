# Getting Started with Unilink {#tutorial_01}

This tutorial shows the current builder-based C++ API for creating a simple TCP client with connection, data, disconnect, and error callbacks.

**Duration**: 10 minutes
**Difficulty**: Beginner
**Prerequisites**: Basic C++ and CMake familiarity

---

## What You'll Build

A small TCP client that:

1. connects to a server
2. prints connection status
3. receives messages through `MessageContext`
4. sends user input until `/quit`

---

## Step 1: Create The Client

Create `my_first_client.cpp`:

<!-- doc-compile: tutorial_getting_started_client -->
```cpp
#include <iostream>
#include <chrono>
#include <string>
#include "unilink/unilink.hpp"

using namespace std::chrono_literals;

int main(int argc, char** argv) {
    std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
    uint16_t port = (argc > 2) ? static_cast<uint16_t>(std::stoi(argv[2])) : 8080;

    auto client = unilink::tcp_client(host, port)
        .retry_interval(2000ms)
        .max_retries(3)
        .on_connect([](const unilink::ConnectionContext&) {
            std::cout << "Connected to server" << std::endl;
        })
        .on_disconnect([](const unilink::ConnectionContext&) {
            std::cout << "Disconnected from server" << std::endl;
        })
        .on_data([](const unilink::MessageContext& ctx) {
            std::cout << "[Server] " << ctx.data() << std::endl;
        })
        .on_error([](const unilink::ErrorContext& ctx) {
            std::cerr << "[Error] " << ctx.message()
                      << " (code=" << static_cast<int>(ctx.code()) << ")" << std::endl;
        })
        .build();

    std::cout << "Connecting to " << host << ":" << port << "..." << std::endl;
    if (!client->start().get()) {
        std::cerr << "Initial connection attempt failed" << std::endl;
        return 1;
    }

    std::cout << "Type messages. Use /quit to exit." << std::endl;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "/quit") break;
        client->send(line);
    }

    client->stop();
    return 0;
}
```

---

## Step 2: Build With CMake

Create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.12)
project(my_first_unilink_app LANGUAGES CXX)

find_package(unilink CONFIG REQUIRED)

add_executable(my_first_client my_first_client.cpp)
target_link_libraries(my_first_client PRIVATE unilink::unilink)
target_compile_features(my_first_client PRIVATE cxx_std_17)
```

Then build:

```bash
cmake -S . -B build
cmake --build build
```

---

## Step 3: Run Against A Test Server

Start a simple server in another terminal:

```bash
nc -l 8080
```

Run the client:

```bash
./build/my_first_client
```

Type a message in the client, then type a reply in the `nc` terminal.

---

## API Patterns Used In This Tutorial

The tutorial code uses context-based callbacks consistently:

- `on_connect(const ConnectionContext&)`
- `on_disconnect(const ConnectionContext&)`
- `on_data(const MessageContext&)`
- `on_error(const ErrorContext&)`

The initial `start()` call returns `std::future<bool>`, so the common pattern is:

```cpp
if (!client->start().get()) {
    // handle startup failure
}
```

After the client is running, use:

- `client->send(...)`
- `client->send_line(...)`
- `client->connected()`
- `client->stop()`

---

## Use The Full Example If You Want More

The repository includes ready-to-build examples:

- [examples/tcp/echo_client.cc](../../../examples/tcp/echo_client.cc)

This tutorial stays smaller than the example sources on purpose.

---

## Next Steps

- [Building a TCP Server](02_tcp_server.md)
- [UDS Communication](03_uds_communication.md)
- [Serial Communication](04_serial_communication.md)
- [UDP Communication](05_udp_communication.md)
- [API Reference](../api_guide.md#tcp-client)

---

**Next**: [Building a TCP Server →](02_tcp_server.md)
