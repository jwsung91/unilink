# Tutorial 03: Local IPC with Unix Domain Sockets (UDS)

In this tutorial, you'll learn how to implement high-performance local inter-process communication using Unix Domain Sockets (UDS) in Unilink.

---

## What are Unix Domain Sockets?

Unix Domain Sockets (UDS) are a data communication endpoint for exchanging data between processes executing on the same host operating system. While TCP/IP is designed for network communication, UDS is optimized for local communication, offering:

- **Lower Latency**: No network stack overhead (routing, checksums, etc.).
- **Higher Throughput**: Efficient memory-to-memory transfers.
- **Security**: Access can be controlled using standard filesystem permissions.

## Step 1: Creating a UDS Server

Creating a UDS server is almost identical to creating a TCP server, but instead of a port number, you provide a filesystem path.

```cpp
#include <iostream>
#include "unilink/unilink.hpp"

int main() {
    // Define the socket path
    std::string socket_path = "/tmp/unilink_echo.sock";

    // Build the UDS server
    auto server = unilink::uds_server(socket_path)
        .unlimited_clients() // Support multiple concurrent connections
        .on_connect([](const unilink::ConnectionContext& ctx) {
            std::cout << "Client connected!" << std::endl;
        })
        .on_data([&](const unilink::MessageContext& ctx) {
            std::string msg(ctx.data());
            std::cout << "Received: " << msg << std::endl;
            
            // Echo back to client
            ctx.reply("Echo: " + msg);
        })
        .on_disconnect([](const unilink::ConnectionContext& ctx) {
            std::cout << "Client disconnected" << std::endl;
        })
        .build();

    // Start the server
    std::cout << "Starting UDS server on " << socket_path << "..." << std::endl;
    server->start();

    std::cout << "Press Enter to stop..." << std::endl;
    std::cin.get();

    server->stop();
    return 0;
}
```

## Step 2: Creating a UDS Client

Similarly, the UDS client connects to the specified socket path.

```cpp
#include <iostream>
#include <string>
#include "unilink/unilink.hpp"

int main() {
    std::string socket_path = "/tmp/unilink_echo.sock";

    // Build the UDS client
    auto client = unilink::uds_client(socket_path)
        .on_connect([](const unilink::ConnectionContext& ctx) {
            std::cout << "Connected to server!" << std::endl;
        })
        .on_data([](const unilink::MessageContext& ctx) {
            std::cout << "Server said: " << ctx.data() << std::endl;
        })
        .on_error([](const unilink::ErrorContext& ctx) {
            std::cerr << "Client error: " << ctx.message() << std::endl;
        })
        .build();

    // Start connecting
    client->start();

    // Interaction loop
    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        if (input == "exit") break;

        if (client->is_connected()) {
            client->send(input);
        } else {
            std::cout << "Connecting..." << std::endl;
        }
    }

    client->stop();
    return 0;
}
```

## Key Considerations for UDS

### Socket Cleanup
Unilink handles the creation and removal of the socket file for you.
- **On Start**: The library automatically removes any existing file at the specified path before binding to prevent "Address already in use" errors.
- **On Stop/Destruction**: The library removes the socket file to leave the filesystem clean.

### Platform Support
- **Linux/macOS**: Native and robust support.
- **Windows**: Supported on Windows 10 (build 17063) and later. For older Windows versions, consider using TCP or Named Pipes.

### Permissions
Since the socket is a file, you can control which users can connect to your server by setting permissions on the socket file or the directory containing it.

---

## Summary
You have successfully implemented a local IPC system using Unilink's UDS support. The Builder API makes it easy to switch between TCP and UDS by simply changing one line of code, while keeping your business logic intact.
