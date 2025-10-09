# Unilink Examples

This directory contains various examples demonstrating how to use the unilink library for different communication protocols and common functionality.

## Structure

- **serial/**: Serial communication examples
- **tcp/**: TCP communication examples  
- **common/**: Common functionality examples

## Quick Start

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
cd tcp/echo
./echo_tcp_server 9000

# TCP echo client
cd tcp/echo
./echo_tcp_client 127.0.0.1 9000

# TCP chat server
cd tcp/chat
./chat_tcp_server 9000

# TCP chat client
cd tcp/chat
./chat_tcp_client 127.0.0.1 9000
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

## Building Examples

```bash
# Build all examples
mkdir build && cd build
cmake ..
make

# Build specific example
make echo_serial
make chat_tcp_server
```

## Example Categories

### Serial Communication
- **echo/**: Serial echo server that echoes received data
- **chat/**: Serial chat application for interactive communication

### TCP Communication
- **echo/**: TCP echo server and client for network echo testing
- **chat/**: TCP chat server and client for network chat
- **multi-chat/**: Multi-client TCP chat server and client

### Common Functionality
- **logging_example**: Demonstrates logging system usage
- **error_handling_example**: Shows error handling system usage

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

#### macOS
```bash
# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install build tools
brew install cmake

# Install Boost libraries
brew install boost

# Install socat for serial testing
brew install socat

# Install serial port tools
brew install minicom
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
- **macOS**: Use `/dev/tty.usbserial-*` for serial ports

For detailed information about each example, see the README.md files in each subdirectory.