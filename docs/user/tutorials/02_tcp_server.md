# Building a TCP Server {#tutorial_02}

This tutorial uses the current `TcpServerBuilder` and `TcpServer` wrapper API to build a simple echo server with context-based callbacks.

**Duration**: 15 minutes
**Difficulty**: Beginner to Intermediate
**Prerequisites**: [Getting Started](01_getting_started.md)

---

## What You'll Build

A TCP server that:

1. listens on a port
2. accepts multiple clients
3. logs connect and disconnect events
4. echoes messages back to the sender

---

## Step 1: Create The Server

Create `echo_server.cpp`:

<!-- doc-compile: tutorial_tcp_server_echo -->
```cpp
#include <iostream>
#include <string>
#include "unilink/unilink.hpp"

class EchoServerApp {
public:
    void start(uint16_t port) {
        server_ = unilink::tcp_server(port)
            .unlimited_clients()
            .port_retry(true)
            .on_connect([this](const unilink::ConnectionContext& ctx) {
                std::cout << "[Connect] client=" << ctx.client_id()
                          << " info=" << ctx.client_info() << std::endl;
                if (server_) {
                    server_->send_to(ctx.client_id(), "Welcome to Echo Server!\n");
                }
            })
            .on_data([this](const unilink::MessageContext& ctx) {
                std::cout << "[Data] client=" << ctx.client_id()
                          << " data=" << ctx.data() << std::endl;
                if (server_) {
                    server_->send_to(ctx.client_id(), "Echo: " + std::string(ctx.data()));
                }
            })
            .on_disconnect([](const unilink::ConnectionContext& ctx) {
                std::cout << "[Disconnect] client=" << ctx.client_id() << std::endl;
            })
            .on_error([](const unilink::ErrorContext& ctx) {
                std::cerr << "[Error] " << ctx.message() << std::endl;
            })
            .build();

        if (!server_->start().get()) {
            throw std::runtime_error("failed to start TCP server");
        }
    }

    void stop() {
        if (server_) {
            server_->broadcast("Server shutting down\n");
            server_->stop();
        }
    }

private:
    std::unique_ptr<unilink::TcpServer> server_;
};

int main() {
    EchoServerApp app;
    app.start(8080);

    std::cout << "Press Enter to stop..." << std::endl;
    std::cin.get();

    app.stop();
    return 0;
}
```

---

## Step 2: Run It

Start the server:

```bash
./echo_server
```

Open another terminal and connect:

```bash
nc localhost 8080
```

Anything you send should be echoed back.

---

## Step 3: Understand The Current Server API

The current server wrapper uses these key methods:

- `start().get()` to verify startup
- `broadcast(...)` to send to all connected clients
- `send_to(client_id, ...)` to reply to one client
- `listening()` to check listener state
- `client_count()` and `connected_clients()` for inspection

The key callback contexts are:

- `ConnectionContext`: client id and client info
- `MessageContext`: client id, message data, and peer info
- `ErrorContext`: error code and message

---

## Client Limits

Choose a client-limit mode before `build()`:

```cpp
unilink::tcp_server(8080).single_client();
unilink::tcp_server(8080).multi_client(8);
unilink::tcp_server(8080).unlimited_clients();
```

For tutorial simplicity, the example above uses `unlimited_clients()`.

---

## Use The Full Example Programs For More

The repository includes ready-to-build examples:

- [examples/tcp/echo_server.cc](../../../examples/tcp/echo_server.cc) — echo back to individual clients
- [examples/tcp/broadcast_server.cc](../../../examples/tcp/broadcast_server.cc) — broadcast to all connected clients

---

## Next Steps

- [UDS Communication](03_uds_communication.md)
- [Serial Communication](04_serial_communication.md)
- [UDP Communication](05_udp_communication.md)
- [API Reference](../api_guide.md#tcp-server)

---

**Previous**: [← Getting Started](01_getting_started.md)
**Next**: [UDS Communication →](03_uds_communication.md)
