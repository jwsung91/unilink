# Unilink Examples

This directory contains runnable examples for the current public `unilink/unilink.hpp` API.

## Structure

- **tutorials/**: Small tutorial companions that mirror the docs
- **common/**: Logging and error handling examples
- **serial/**: Serial communication examples
- **tcp/**: TCP communication examples
- **udp/**: UDP communication examples
- **uds/**: Unix domain socket examples

## Quick Start

### Build Examples

```bash
# From project root
mkdir -p build
cd build
cmake ..
cmake --build .
```

### Serial Communication

```bash
# Serial echo server
cd serial/echo
./echo_serial /dev/ttyUSB0 115200

# Serial chat
cd serial/chat
./chat_serial /dev/ttyUSB0 115200

# Testing with socat (virtual serial ports)
# Terminal 1: Create virtual serial pair
socat -d -d pty,raw,echo=0,link=/tmp/ttyA pty,raw,echo=0,link=/tmp/ttyB

# Terminal 2: Run echo server on first port
./echo_serial /tmp/ttyA 115200

# Terminal 3: Connect to second port
socat - /tmp/ttyB
```

### TCP Communication

```bash
# TCP echo server
cd tcp/single-echo
./echo_tcp_server 9000

# TCP echo client
cd tcp/single-echo
./echo_tcp_client 127.0.0.1 9000

# TCP chat server
cd tcp/single-chat
./chat_tcp_server 9000

# TCP chat client
cd tcp/single-chat
./chat_tcp_client 127.0.0.1 9000
```

### UDS Communication

```bash
# UDS echo server
cd uds
./echo_uds_server
# or
./echo_uds_server /tmp/my_socket.sock

# UDS echo client
cd uds
./echo_uds_client
# or
./echo_uds_client /tmp/my_socket.sock
```

### UDP Communication

```bash
# UDP receiver (listens on 9000)
cd udp
./udp_receiver

# UDP sender (sends to 127.0.0.1:9000)
cd udp
./udp_sender
```

### Common Functionality

```bash
# Logging example
cd common
./logging_example

# Error handling example
cd common
./error_handling_example
```

## Example Categories

### Tutorial Companions

- **tutorials/**: Minimal tutorial programs kept in sync with the docs

### Common Functionality

- **logging_example**: Shows basic logger setup and logging hooks
- **error_handling_example**: Shows `on_error(...)` callbacks and failed start/connect flows

### Serial Communication

- **echo/**: Serial echo server that echoes received data
- **chat/**: Serial chat application for interactive communication

### TCP Communication

- **single-echo/**: TCP echo server and client for network echo testing
- **single-chat/**: TCP chat server and client for network chat
- **multi-chat/**: Multi-client chat server and client

### UDS Communication

- **uds/**: Unix Domain Socket echo server and client for local IPC testing

## Prerequisites

- C++17 or later
- CMake 3.10 or later
- Boost libraries (for TCP functionality)
- Serial port access (for serial examples)

### Required Tools Installation

#### Ubuntu/Debian

```bash
# Install build tools
sudo apt update
sudo apt install build-essential cmake

# Install Boost libraries
sudo apt install libboost-all-dev

# Install socat for serial testing
sudo apt install socat

# Install serial port tools
sudo apt install minicom screen
```

#### CentOS/RHEL/Fedora

```bash
# Install build tools
sudo yum groupinstall "Development Tools"
sudo yum install cmake

# Install Boost libraries
sudo yum install boost-devel

# Install socat for serial testing
sudo yum install socat

# Install serial port tools
sudo yum install minicom screen
```

#### Windows

```bash
# Install vcpkg (package manager)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
./vcpkg integrate install

# Install Boost libraries
./vcpkg install boost

# Install socat (via WSL or Cygwin)
# Or use PuTTY for serial communication
```

## Platform Notes

- **Linux**: Use `/dev/ttyUSB0`, `/dev/ttyACM0` for serial ports
- **Windows**: Use `COM3`, `COM4` for serial ports
- **UDS**: Unix domain sockets are intended for Unix-like platforms

For detailed information about each example, see the README.md files in each subdirectory.
