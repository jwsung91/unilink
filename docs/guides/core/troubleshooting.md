# Troubleshooting Guide

Common issues and solutions when using unilink.

---

## Table of Contents

1. [Connection Issues](#connection-issues)
2. [Compilation Errors](#compilation-errors)
3. [Runtime Errors](#runtime-errors)
4. [Performance Issues](#performance-issues)
5. [Memory Issues](#memory-issues)
6. [Thread Safety Issues](#thread-safety-issues)
7. [Debugging Tips](#debugging-tips)

---

## Connection Issues

### Problem: Connection Refused

**Symptoms:**
```
✗ Error: Connection refused
```

**Possible Causes & Solutions:**

#### 1. Server Not Running
```bash
# Check if server is running
netstat -an | grep 8080
# or
lsof -i :8080
```

**Solution:** Start the server before connecting client.

#### 2. Wrong Host/Port
```cpp
// Check your configuration
auto client = tcp_client("127.0.0.1", 8080)  // Correct port?
    .build();
```

**Solution:** Verify server address and port number.

#### 3. Firewall Blocking
```bash
# Check firewall rules (Linux)
sudo iptables -L | grep 8080

# Allow port (Ubuntu)
sudo ufw allow 8080
```

---

### Problem: Connection Timeout

**Symptoms:**
```
✗ Error: Connection timeout
```

**Possible Causes & Solutions:**

#### 1. Network Unreachable
```bash
# Test connectivity
ping server.com
telnet server.com 8080
```

**Solution:** Check network connectivity, DNS resolution.

#### 2. Server Overloaded
**Solution:** Increase retry interval:
```cpp
auto client = tcp_client("server.com", 8080)
    .retry_interval(10000)  // Wait 10 seconds
    .build();
```

#### 3. Slow Network
**Solution:** Increase timeout (requires custom implementation):
```cpp
// Set socket options for longer timeout
boost::asio::socket_base::linger option(true, 30);
socket.set_option(option);
```

---

### Problem: Connection Drops Randomly

**Symptoms:**
- Client disconnects unexpectedly
- `on_disconnect()` called without `stop()`

**Possible Causes & Solutions:**

#### 1. Network Instability
```cpp
// Enable auto-reconnection
auto client = tcp_client("server.com", 8080)
    .retry_interval(3000)  // Retry every 3 seconds
    .on_disconnect([]() {
        log_info("Disconnected, will auto-retry");
    })
    .build();
```

#### 2. Server Closing Connection
```cpp
// Log disconnect reason
.on_disconnect([this]() {
    // Check if stop() was called
    if (!intentional_disconnect_) {
        log_warning("Unexpected disconnect from server");
    }
})
```

#### 3. Keep-Alive Not Set
```cpp
// Implement heartbeat
std::thread heartbeat_thread([this]() {
    while (running_) {
        if (client_->is_connected()) {
            client_->send("PING\n");
        }
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
});
```

---

### Problem: Port Already in Use

**Symptoms:**
```
✗ Error: Address already in use
```

**Solutions:**

#### 1. Kill Existing Process
```bash
# Find process using port
lsof -i :8080
# or
netstat -tulpn | grep 8080

# Kill process
kill -9 <PID>
```

#### 2. Use Different Port
```cpp
auto server = tcp_server(8081)  // Try different port
    .build();
```

#### 3. Enable Port Retry
```cpp
auto server = tcp_server(8080)
    .enable_port_retry(true, 5, 1000)  // Retry 5 times
    .build();
```

#### 4. Set SO_REUSEADDR (Advanced)
```cpp
// Allow immediate port reuse
boost::asio::socket_base::reuse_address option(true);
acceptor.set_option(option);
```

---

## Compilation Errors

### Problem: unilink/unilink.hpp Not Found

**Symptoms:**
```
fatal error: unilink/unilink.hpp: No such file or directory
```

**Solutions:**

#### 1. Install unilink
```bash
cd interface-socket
cmake -S . -B build
sudo cmake --install build
```

#### 2. Add Include Path
```bash
# CMake
target_include_directories(your_app PRIVATE /path/to/unilink/include)

# g++
g++ -I/path/to/unilink/include ...
```

#### 3. Use as Subdirectory
```cmake
# CMakeLists.txt
add_subdirectory(unilink)
target_link_libraries(your_app PRIVATE unilink)
```

---

### Problem: Undefined Reference to unilink Symbols

**Symptoms:**
```
undefined reference to `unilink::tcp_client(std::string const&, unsigned short)'
```

**Solutions:**

#### 1. Link unilink Library
```bash
# g++
g++ main.cpp -o app -lunilink -lboost_system -pthread

# CMake
target_link_libraries(your_app PRIVATE unilink Boost::system pthread)
```

#### 2. Check Library Path
```bash
# Add to LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Or add to ldconfig
sudo sh -c 'echo "/usr/local/lib" > /etc/ld.so.conf.d/unilink.conf'
sudo ldconfig
```

---

### Problem: Boost Not Found

**Symptoms:**
```
CMake Error: Could not find Boost
```

**Solutions:**

#### Ubuntu/Debian
```bash
sudo apt install libboost-all-dev
```

#### macOS
```bash
brew install boost
```

#### Windows (vcpkg)
```bash
vcpkg install boost
```

#### Manual Boost Path
```cmake
set(BOOST_ROOT /path/to/boost)
find_package(Boost REQUIRED COMPONENTS system)
```

---

## Runtime Errors

### Problem: Segmentation Fault

**Symptoms:**
```
Segmentation fault (core dumped)
```

**Debugging Steps:**

#### 1. Enable Core Dumps
```bash
ulimit -c unlimited
./your_app
# If crash occurs, core dump is created

# Analyze with gdb
gdb ./your_app core
(gdb) bt  # backtrace
```

#### 2. Use AddressSanitizer
```bash
# Compile with sanitizer
cmake -DUNILINK_ENABLE_SANITIZERS=ON ..
cmake --build .

# Run
./your_app
# Will show detailed error if memory issue
```

#### 3. Common Causes

**Dangling Pointer:**
```cpp
// BAD
auto* client = client_ptr.get();
client_ptr.reset();  // Deletes object
client->send("data");  // CRASH - dangling pointer

// GOOD
auto client = client_ptr;  // Keep shared_ptr alive
client->send("data");
```

**Use After Stop:**
```cpp
// BAD
client->stop();
client->send("data");  // CRASH - client stopped

// GOOD
if (client->is_connected()) {
    client->send("data");
}
```

---

### Problem: Callbacks Not Being Called

**Symptoms:**
- `on_connect()` never called
- `on_data()` not receiving data

**Possible Causes & Solutions:**

#### 1. Callback Not Registered
```cpp
// BAD - Forgot to register callback
auto client = tcp_client("server.com", 8080)
    .build();  // No on_data callback!

// GOOD
auto client = tcp_client("server.com", 8080)
    .on_data([](const std::string& data) {
        std::cout << data << std::endl;
    })
    .build();
```

#### 2. Client Not Started
```cpp
// BAD - Forgot to start
auto client = tcp_client("server.com", 8080)
    .build();
// ... client never starts!

// GOOD - Always call start explicitly
auto client = tcp_client("server.com", 8080)
    .build();
client->start();  // Start the connection
```

#### 3. Application Exits Too Quickly
```cpp
// BAD
int main() {
    auto client = tcp_client("server.com", 8080)
        .on_connect([]() { std::cout << "Connected!\n"; })
        .build();
    client->start();
    
    return 0;  // Exits immediately before connection!
}

// GOOD
int main() {
    auto client = tcp_client("server.com", 8080)
        .on_connect([]() { std::cout << "Connected!\n"; })
        .build();
    client->start();
    
    std::this_thread::sleep_for(std::chrono::seconds(10));  // Wait
    return 0;
}
```

---

## Performance Issues

### Problem: High CPU Usage

**Symptoms:**
- Process using 100% CPU
- System becomes slow

**Possible Causes & Solutions:**

#### 1. Busy Loop in Callback
```cpp
// BAD - Blocks I/O thread
.on_data([](const std::string& data) {
    while (processing) {  // Busy loop!
        process();
    }
})

// GOOD - Process asynchronously
.on_data([this](const std::string& data) {
    message_queue_.push(data);  // Queue for processing
})
```

#### 2. Too Many Retries
```cpp
// BAD - Retries too often
.retry_interval(10)  // Retry every 10ms!

// GOOD
.retry_interval(3000)  // Retry every 3 seconds
```

#### 3. Excessive Logging
```cpp
// BAD - Log every byte
.on_data([](const std::string& data) {
    for (auto byte : data) {
        logger.debug("byte", "received", std::to_string(byte));
    }
})

// GOOD - Log summary
.on_data([](const std::string& data) {
    logger.debug("data", "received", "Received " + std::to_string(data.size()) + " bytes");
})
```

---

### Problem: High Memory Usage

**Symptoms:**
- Memory usage keeps growing
- Out of memory errors

**Solutions:**

#### 1. Enable Memory Tracking (Debug)
```bash
cmake -DUNILINK_ENABLE_MEMORY_TRACKING=ON ..
cmake --build .
./your_app

# Check memory report
```

#### 2. Fix Memory Leaks
```cpp
// BAD - Circular reference
class Client {
    std::shared_ptr<Handler> handler_;
};
class Handler {
    std::shared_ptr<Client> client_;  // Circular!
};

// GOOD - Break cycle with weak_ptr
class Handler {
    std::weak_ptr<Client> client_;  // Weak reference
};
```

#### 3. Limit Buffer Sizes
```cpp
// Limit message queue size
std::deque<std::string> message_queue_;
const size_t MAX_QUEUE_SIZE = 1000;

void add_message(const std::string& msg) {
    if (message_queue_.size() >= MAX_QUEUE_SIZE) {
        message_queue_.pop_front();  // Drop oldest
    }
    message_queue_.push_back(msg);
}
```

---

### Problem: Slow Data Transfer

**Symptoms:**
- Low throughput
- Messages take long to arrive

**Solutions:**

#### 1. Batch Small Messages
```cpp
// BAD - Send each character
for (char c : message) {
    client->send(std::string(1, c));  // Slow!
}

// GOOD - Send whole message
client->send(message);
```

#### 2. Use Binary Protocol
```cpp
// Instead of text: "LENGTH:1024\nDATA:..."
// Use binary: [4 bytes length][data]

std::vector<uint8_t> create_binary_message(const std::string& data) {
    std::vector<uint8_t> msg;
    uint32_t len = data.size();
    msg.insert(msg.end(), (uint8_t*)&len, (uint8_t*)&len + 4);
    msg.insert(msg.end(), data.begin(), data.end());
    return msg;
}
```

#### 3. Enable Async Logging
```cpp
// Don't let logging slow down I/O
Logger::instance().enable_async(true);
```

---

## Memory Issues

### Problem: Memory Leak Detected

**Symptoms:**
```
Memory leak detected: 1024 bytes at 0x...
```

**Debugging:**

```bash
# Build with AddressSanitizer
cmake -DUNILINK_ENABLE_SANITIZERS=ON ..
cmake --build .

# Run
ASAN_OPTIONS=detect_leaks=1 ./your_app

# Or use valgrind
valgrind --leak-check=full ./your_app
```

**Common Causes:**

```cpp
// 1. Not calling stop()
{
    auto client = tcp_client("server.com", 8080).build();
    client->start();
    // Forgot to call client->stop()
}

// 2. Circular shared_ptr
// See "High Memory Usage" section above

// 3. Not clearing callbacks
server->clear_callbacks();  // Clear before destruction
```

---

## Thread Safety Issues

### Problem: Race Condition / Data Corruption

**Symptoms:**
- Occasional crashes
- Corrupted data
- Inconsistent state

**Solutions:**

#### 1. Protect Shared State
```cpp
// BAD - No protection
std::map<size_t, ClientInfo> clients_;

void add_client(size_t id) {
    clients_[id] = ClientInfo{};  // UNSAFE!
}

// GOOD - Use mutex
std::mutex clients_mutex_;
std::map<size_t, ClientInfo> clients_;

void add_client(size_t id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_[id] = ClientInfo{};  // Safe
}
```

#### 2. Use Thread-Safe Containers
```cpp
#include <unilink/concurrency/thread_safe_state.hpp>

unilink::common::ThreadSafeState<State> state_;
// All operations are thread-safe
```

---

## Debugging Tips

### Enable Debug Logging

```cpp
// In main() or initialization
void setup_debugging() {
    // Set debug log level
    unilink::common::Logger::instance().set_level(unilink::common::LogLevel::DEBUG);
    unilink::common::Logger::instance().set_console_output(true);
    unilink::common::Logger::instance().set_file_output("debug.log");
    
    // Enable error handler
    unilink::common::ErrorHandler::instance().set_min_error_level(
        unilink::common::ErrorLevel::INFO
    );
    unilink::common::ErrorHandler::instance().register_callback(
        [](const unilink::common::ErrorInfo& error) {
            std::cerr << "[ERROR] " << error.get_summary() << std::endl;
        }
    );
}
```

### Use GDB for Debugging

```bash
# Compile with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .

# Run with gdb
gdb ./your_app

# Useful gdb commands
(gdb) break main
(gdb) run
(gdb) next
(gdb) step
(gdb) print variable_name
(gdb) bt  # backtrace
(gdb) info threads
```

### Network Debugging with tcpdump

```bash
# Capture traffic on port 8080
sudo tcpdump -i any -nn port 8080 -A

# Save to file
sudo tcpdump -i any port 8080 -w capture.pcap

# View with Wireshark
wireshark capture.pcap
```

### Test with netcat

```bash
# Server mode - listen on port 8080
nc -l 8080

# Client mode - connect to server
nc localhost 8080

# Send file
nc localhost 8080 < test_data.txt
```

---

## Getting Help

If you're still experiencing issues:

1. **Check Examples**: Look at `examples/` directory
2. **Read API Guide**: See `docs/reference/API_GUIDE.md`
3. **Search Issues**: https://github.com/jwsung91/unilink/issues
4. **Ask Community**: Create a new issue with:
   - Minimal reproducible example
   - Error messages / logs
   - Environment details (OS, compiler, versions)
   - What you've tried so far

---

**See Also:**
- [Best Practices](best_practices.md)
- [Performance Tuning](performance_tuning.md)
- [API Reference](../reference/API_GUIDE.md)
