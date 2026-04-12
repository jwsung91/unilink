# Serial Communication

This tutorial walks through a practical serial workflow with `unilink`: open a device, receive lines, send commands, and test locally with virtual ports before connecting real hardware.

**Duration**: 15 minutes  
**Difficulty**: Beginner to Intermediate  
**Prerequisites**: [Getting Started](01_getting_started.md)

---

## What You'll Build

A simple serial terminal that:

1. opens a serial device
2. logs connect and disconnect events
3. prints incoming data
4. sends user input back over the serial link

---

## Step 1: Choose A Device Path

Typical serial device names:

- Linux USB adapter: `/dev/ttyUSB0`
- Linux CDC/Arduino style device: `/dev/ttyACM0`
- Windows serial port: `COM3`

If you are testing on Linux without hardware, use `socat` to create a virtual pair:

```bash
socat -d -d pty,raw,echo=0,link=/tmp/ttyA pty,raw,echo=0,link=/tmp/ttyB
```

This creates two connected ports, `/tmp/ttyA` and `/tmp/ttyB`.

---

## Step 2: Create A Minimal Serial Terminal

<!-- doc-compile: tutorial_serial_terminal -->
```cpp
#include <iostream>
#include <chrono>
#include <string>
#include "unilink/unilink.hpp"

using namespace std::chrono_literals;

int main(int argc, char** argv) {
    std::string device = (argc > 1) ? argv[1] : "/dev/ttyUSB0";
    uint32_t baud = (argc > 2) ? static_cast<uint32_t>(std::stoul(argv[2])) : 115200;

    auto serial = unilink::serial(device, baud)
        .on_connect([](const unilink::ConnectionContext&) {
            std::cout << "Serial port opened" << std::endl;
        })
        .on_disconnect([](const unilink::ConnectionContext&) {
            std::cout << "Serial port closed" << std::endl;
        })
        .on_data([](const unilink::MessageContext& ctx) {
            std::cout << "[RX] " << ctx.data() << std::endl;
        })
        .on_error([](const unilink::ErrorContext& ctx) {
            std::cerr << "[ERROR] " << ctx.message() << std::endl;
        })
        .build();

    if (!serial->start().get()) {
        std::cerr << "Failed to open serial device" << std::endl;
        return 1;
    }

    std::cout << "Type messages. Use /quit to exit." << std::endl;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "/quit") break;
        if (serial->is_connected()) {
            serial->send(line);
        }
    }

    serial->stop();
    return 0;
}
```

---

## Step 3: Build And Run

```bash
g++ -std=c++17 serial_terminal.cpp -o serial_terminal -lunilink -lboost_system -pthread
```

Run against a real device:

```bash
./serial_terminal /dev/ttyUSB0 115200
```

Run against a virtual port:

```bash
./serial_terminal /tmp/ttyA 115200
```

---

## Step 4: Test With A Second Terminal

If you used the `socat` pair above:

**Terminal 1**

```bash
./serial_terminal /tmp/ttyA 115200
```

**Terminal 2**

```bash
./serial_terminal /tmp/ttyB 115200
```

Messages typed in one terminal should appear in the other.

---

## Common Adjustments

You can tune the serial wrapper before `build()`:

```cpp
using namespace std::chrono_literals;
auto serial = unilink::serial("/dev/ttyUSB0", 115200)
    .retry_interval(1000ms)
    .build();
```

At runtime, you can also adjust settings on the wrapper:

```cpp
serial->set_baud_rate(9600);
serial->set_data_bits(8);
serial->set_stop_bits(1);
serial->set_parity("none");
serial->set_flow_control("none");
```

---

## When To Use The Example Programs Instead

If you want a fuller interactive sample, use the existing example programs:

- [examples/serial/README.md](../../examples/serial/README.md)
- [examples/serial/echo/README.md](../../examples/serial/echo/README.md)
- [examples/serial/chat/README.md](../../examples/serial/chat/README.md)

Those examples are a better fit for device bring-up and manual testing than this short tutorial.

---

## Next Steps

- [UDP Communication](05_udp_communication.md)
- [API Reference](../reference/api_guide.md#serial-communication)
- [Examples Directory](../../examples/serial/README.md)

---

**Previous**: [← UDS Communication](03_uds_communication.md)  
**Next**: [UDP Communication →](05_udp_communication.md)
