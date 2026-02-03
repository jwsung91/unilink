# Tutorial 1: Getting Started with Unilink

This tutorial will guide you through creating your first application with unilink.

**Duration**: 15 minutes  
**Difficulty**: Beginner  
**Prerequisites**: Basic C++ knowledge, CMake installed

---

## What You'll Build

A simple TCP client that:
1. Connects to a server
2. Sends a message
3. Receives a response
4. Handles disconnections gracefully

---

## Step 1: Install Dependencies

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install -y build-essential cmake libboost-dev libboost-system-dev
```

### macOS
```bash
brew install cmake boost
```

### Windows (vcpkg)
```bash
vcpkg install boost
```

---

## Step 2: Install Unilink

### Option A: From Source
```bash
git clone https://github.com/jwsung91/unilink.git
cd unilink
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build
```

### Option B: Use as Subdirectory
```cmake
# In your CMakeLists.txt
add_subdirectory(unilink)
target_link_libraries(your_app PRIVATE unilink)
```

---

## Step 3: Create Your First Client

Create a file named `my_first_client.cpp`:

```cpp
#include <iostream>
#include <thread>
#include <chrono>
#include "unilink/unilink.hpp"

int main() {
    std::cout << "=== My First Unilink Client ===" << std::endl;
    
    // Step 1: Create a TCP client
    auto client = unilink::tcp_client("127.0.0.1", 8080)
        
        // Step 2: Handle connection event
        .on_connect([]() {
            std::cout << "✓ Connected to server!" << std::endl;
        })
        
        // Step 3: Handle incoming data
        .on_data([](const std::string& data) {
            std::cout << "✓ Received: " << data << std::endl;
        })
        
        // Step 4: Handle disconnection
        .on_disconnect([]() {
            std::cout << "✗ Disconnected from server" << std::endl;
        })
        
        // Step 5: Handle errors
        .on_error([](const std::string& error) {
            std::cerr << "✗ Error: " << error << std::endl;
        })
        
        // Step 6: Configure auto-reconnection (default is 3000ms)
        // .retry_interval(3000)  // This is the default, so we can omit it
        
        // Build the client
        .build();
    
    // Step 7: Start the connection explicitly
    client->start();
    
    // Wait for connection (up to 5 seconds)
    std::cout << "Waiting for connection..." << std::endl;
    for (int i = 0; i < 50; ++i) {
        if (client->is_connected()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Send a message
    if (client->is_connected()) {
        std::cout << "Sending message..." << std::endl;
        client->send("Hello from unilink!");
    } else {
        std::cerr << "Failed to connect. Is the server running?" << std::endl;
        return 1;
    }
    
    // Keep running for 10 seconds
    std::cout << "Running for 10 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    std::cout << "Shutting down..." << std::endl;
    return 0;
}
```

---

## Step 4: Create CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.10)
project(MyFirstUnilinkApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find unilink (adjust path if needed)
find_package(unilink REQUIRED)

# Create executable
add_executable(my_first_client my_first_client.cpp)

# Link libraries
target_link_libraries(my_first_client
    PRIVATE
        unilink
        Boost::system
        pthread
)
```

---

## Step 5: Build Your Application

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Or with direct compilation:
```bash
g++ -std=c++17 my_first_client.cpp \
    -o my_first_client \
    -lunilink \
    -lboost_system \
    -pthread
```

---

## Step 6: Test Your Application

### Start a Test Server

**Terminal 1** (using netcat):
```bash
nc -l 8080
```

Or use the provided example server:
```bash
cd interface-socket/build/examples/tcp/single-echo
./echo_tcp_server 8080
```

### Run Your Client

**Terminal 2**:
```bash
./my_first_client
```

**Expected Output**:
```
=== My First Unilink Client ===
Waiting for connection...
✓ Connected to server!
Sending message...
✓ Received: Hello from unilink!
Running for 10 seconds...
Shutting down...
```

---

## Step 7: Understanding the Code

### 1. Builder Pattern
```cpp
auto client = unilink::tcp_client("127.0.0.1", 8080)
    .on_connect(...)
    .on_data(...)
    .build();
```
The builder pattern provides a fluent, chainable API for configuration.

### 2. Callbacks
```cpp
.on_connect([]() { /* called when connected */ })
.on_data([](const std::string& data) { /* called when data received */ })
```
Callbacks are invoked automatically by the library.

### 3. Automatic Reconnection
```cpp
.retry_interval(3000)  // Retry every 3 seconds (default)
```
After calling `client->start()`, the client will automatically attempt to reconnect if disconnected.

### 4. Thread Safety
All callbacks are executed in a thread-safe manner. You can safely access shared data from callbacks using proper synchronization.

---

## Common Issues

### Issue 1: Connection Refused
```
✗ Error: Connection refused
```

**Solution**: Make sure the server is running on the correct port.

### Issue 2: Port Already in Use
```
✗ Error: Address already in use
```

**Solution**: The port is being used by another application. Choose a different port or stop the other application.

### Issue 3: Compilation Error - unilink not found
```
fatal error: unilink/unilink.hpp: No such file or directory
```

**Solution**: 
```bash
# Make sure unilink is installed
sudo cmake --install build

# Or add include path
g++ -I/path/to/unilink/include ...
```

---

## Next Steps

Congratulations! You've created your first unilink application. 

**Continue learning:**
- [Tutorial 2: Building a TCP Server](02_tcp_server.md)
- [Tutorial 3: Serial Communication](03_serial_communication.md)
- [Tutorial 4: Error Handling](04_error_handling.md)

**Explore more:**
- [API Reference](../reference/API_GUIDE.md)
- [Best Practices](../guides/best_practices.md)
- [Examples Directory](../../examples/)

---

## Full Example Code

✅ **Ready-to-compile examples are available!**

**Location**: `examples/tutorials/getting_started/`

| File | Description |
|------|-------------|
| `simple_client.cpp` | Minimal 30-second example |
| `my_first_client.cpp` | Complete walkthrough from this tutorial |

**Build and run**:
```bash
# From project root
cmake --build build --target tutorial_my_first_client
./build/examples/tutorials/tutorial_my_first_client
```

See [examples/tutorials/README.md](../../examples/tutorials/README.md) for detailed build instructions.

---

**Next Tutorial**: [Building a TCP Server →](02_tcp_server.md)

