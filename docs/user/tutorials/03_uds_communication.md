# UDS Communication

This tutorial shows the current UDS client/server API using `uds_server(...)` and `uds_client(...)` from `unilink/unilink.hpp`.

**Duration**: 10 minutes
**Difficulty**: Beginner to Intermediate
**Prerequisites**: [Building a TCP Server](02_tcp_server.md)

---

## What You'll Build

A local echo service that:

1. binds a Unix domain socket path
2. accepts multiple local clients
3. echoes each message back to the sender

---

## Step 1: Create A UDS Server

<!-- doc-compile: tutorial_uds_server -->
```cpp
#include <iostream>
#include <string>
#include "unilink/unilink.hpp"

int main() {
    const std::string socket_path = "/tmp/unilink_echo.sock";

    std::unique_ptr<unilink::UdsServer> server;
    server = unilink::uds_server(socket_path)
        .unlimited_clients()
        .on_connect([](const unilink::ConnectionContext& ctx) {
            std::cout << "Client connected: " << ctx.client_id()
                      << " info=" << ctx.client_info() << std::endl;
        })
        .on_data([&server](const unilink::MessageContext& ctx) {
            std::cout << "Received: " << ctx.data() << std::endl;
            server->send_to(ctx.client_id(), "Echo: " + std::string(ctx.data()));
        })
        .on_disconnect([](const unilink::ConnectionContext& ctx) {
            std::cout << "Client disconnected: " << ctx.client_id() << std::endl;
        })
        .on_error([](const unilink::ErrorContext& ctx) {
            std::cerr << "[Error] " << ctx.message() << std::endl;
        })
        .build();

    if (!server->start().get()) {
        std::cerr << "Failed to start UDS server" << std::endl;
        return 1;
    }

    std::cout << "Listening on " << socket_path << std::endl;
    std::cin.get();
    server->stop();
    return 0;
}
```

---

## Step 2: Create A UDS Client

<!-- doc-compile: tutorial_uds_client -->
```cpp
#include <iostream>
#include <string>
#include "unilink/unilink.hpp"

int main() {
    const std::string socket_path = "/tmp/unilink_echo.sock";

    auto client = unilink::uds_client(socket_path)
        .on_connect([](const unilink::ConnectionContext&) {
            std::cout << "Connected to UDS server" << std::endl;
        })
        .on_data([](const unilink::MessageContext& ctx) {
            std::cout << "[Server] " << ctx.data() << std::endl;
        })
        .on_error([](const unilink::ErrorContext& ctx) {
            std::cerr << "[Error] " << ctx.message() << std::endl;
        })
        .build();

    if (!client->start().get()) {
        std::cerr << "Failed to connect to UDS server" << std::endl;
        return 1;
    }

    std::string input;
    while (std::getline(std::cin, input)) {
        if (input == "/quit") break;
        client->send(input);
    }

    client->stop();
    return 0;
}
```

---

## Why Use UDS Instead Of TCP

UDS is useful when both processes are on the same machine and you want:

- low local IPC overhead
- filesystem-based access control
- a socket path instead of a TCP port

The callback model and wrapper lifecycle are intentionally very similar to the TCP API.

---

## Operational Notes

- The socket path should usually live under `/tmp` or another writable runtime directory.
- Old socket files can cause bind failures if a process crashes before cleanup.
- On Linux and macOS, UDS is a natural choice for local service-to-service IPC.

---

## Next Steps

- [Serial Communication](04_serial_communication.md)
- [UDP Communication](05_udp_communication.md)
- [API Reference](../api_guide.md#uds-communication)
- [Examples Directory](../../../examples/uds/README.md)

---

**Previous**: [← Building a TCP Server](02_tcp_server.md)
**Next**: [Serial Communication →](04_serial_communication.md)
