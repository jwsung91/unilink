# UDP Communication

This tutorial covers the basic UDP workflow in `unilink`: bind a receiver, configure a sender, and observe how the wrapper behaves with connectionless traffic.

**Duration**: 10 minutes  
**Difficulty**: Beginner to Intermediate  
**Prerequisites**: [Getting Started](01_getting_started.md)

---

## What You'll Build

A two-process UDP setup:

1. one receiver bound to a local port
2. one sender with a configured remote endpoint
3. simple message exchange between the two

---

## Step 1: Create A Receiver

<!-- doc-compile: tutorial_udp_receiver -->
```cpp
#include <iostream>
#include "unilink/unilink.hpp"

int main() {
    auto receiver = unilink::udp(9000)
        .on_data([](const unilink::MessageContext& ctx) {
            std::cout << "[RX] " << ctx.data() << std::endl;
        })
        .on_error([](const unilink::ErrorContext& ctx) {
            std::cerr << "[ERROR] " << ctx.message() << std::endl;
        })
        .build();

    if (!receiver->start().get()) {
        std::cerr << "Failed to start receiver" << std::endl;
        return 1;
    }

    std::cout << "Listening on UDP port 9000. Press Enter to stop." << std::endl;
    std::cin.get();
    receiver->stop();
    return 0;
}
```

---

## Step 2: Create A Sender

<!-- doc-compile: tutorial_udp_sender -->
```cpp
#include <iostream>
#include <string>
#include "unilink/unilink.hpp"

int main() {
    auto sender = unilink::udp(0)
        .remote("127.0.0.1", 9000)
        .on_connect([](const unilink::ConnectionContext&) {
            std::cout << "UDP sender ready" << std::endl;
        })
        .on_error([](const unilink::ErrorContext& ctx) {
            std::cerr << "[ERROR] " << ctx.message() << std::endl;
        })
        .build();

    sender->start();

    std::cout << "Type messages. Use /quit to exit." << std::endl;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "/quit") break;
        sender->send(line);
    }

    sender->stop();
    return 0;
}
```

---

## Step 3: Build The Two Programs

If you saved the snippets as `udp_receiver.cpp` and `udp_sender.cpp`:

```bash
g++ -std=c++17 udp_receiver.cpp -o udp_receiver -lunilink -lboost_system -pthread
g++ -std=c++17 udp_sender.cpp -o udp_sender -lunilink -lboost_system -pthread
```

---

## Step 4: Run Both Programs

**Terminal 1**

```bash
./udp_receiver
```

**Terminal 2**

```bash
./udp_sender
```

Type messages in the sender terminal and verify that the receiver prints them.

---

## What Is Different About UDP

Unlike TCP:

- there is no session handshake
- delivery is not guaranteed
- message ordering is not guaranteed
- the sender can write as soon as it has a destination

For simple local tests, UDP is useful when you want low overhead and can tolerate datagram semantics.

---

## Practical Notes

- `unilink::udp(0)` uses an ephemeral local port for the sender.
- `remote(host, port)` configures the default destination for `send()`.
- The receiver can run without a predefined remote peer.
- If you need more operational examples, the protocol-specific examples are richer than this tutorial.

---

## Use The Full Examples For Repeated Testing

For ready-to-run maintained examples, use:

- [examples/udp/README.md](../../examples/udp/README.md)
- [examples/udp/udp_receiver.cpp](../../examples/udp/udp_receiver.cpp)
- [examples/udp/udp_sender.cpp](../../examples/udp/udp_sender.cpp)

Those examples intentionally stay minimal too: one receiver bound to `9000` and one sender targeting `127.0.0.1:9000`.

---

## Next Steps

- [API Reference](../reference/api_guide.md#udp-communication)
- [Performance Guide](../guides/advanced/performance.md)
- [Examples Directory](../../examples/udp/README.md)

---

**Previous**: [← Serial Communication](04_serial_communication.md)
