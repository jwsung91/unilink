# 📘 New Unilink API Guide

This guide explains how to use the new high-level API in unilink library.

## 🚀 Quick Start

### TCP Server
```cpp
#include "unilink/unilink.hpp"

int main() {
    auto server = unilink::tcp_server(9000)
        .on_data([](const std::string& data) {
            std::cout << "Echo: " << data;
        })
        .on_connect([]() {
            std::cout << "Client connected!" << std::endl;
        })
        .auto_manage()
        .build();
    
    std::promise<void>().get_future().wait();
    return 0;
}
```

### TCP Client
```cpp
#include "unilink/unilink.hpp"

int main() {
    auto client = unilink::tcp_client("127.0.0.1", 9000)
        .on_data([](const std::string& data) {
            std::cout << "Received: " << data;
        })
        .on_connect([]() {
            std::cout << "Connected!" << std::endl;
        })
        .retry_interval(std::chrono::seconds(3))
        .auto_manage()
        .build();
    
    std::promise<void>().get_future().wait();
    return 0;
}
```

### Serial Communication
```cpp
#include "unilink/unilink.hpp"

int main() {
    auto serial = unilink::serial("/dev/ttyUSB0", 9600)
        .on_data([](const std::string& data) {
            std::cout << "Serial: " << data;
        })
        .on_connect([]() {
            std::cout << "Serial connected!" << std::endl;
        })
        .data_bits(8)
        .parity("none")
        .auto_manage()
        .build();
    
    std::promise<void>().get_future().wait();
    return 0;
}
```

## 🔧 Configuration Options

### TCP Server Options
- `.port(uint16_t)` - Server port
- `.max_connections(size_t)` - Maximum connections
- `.timeout(std::chrono::milliseconds)` - Connection timeout
- `.buffer_size(size_t)` - Buffer size

### TCP Client Options
- `.host(const std::string&)` - Server host
- `.port(uint16_t)` - Server port
- `.retry_interval(std::chrono::milliseconds)` - Retry interval
- `.max_retries(int)` - Maximum retry attempts
- `.connection_timeout(std::chrono::milliseconds)` - Connection timeout
- `.keep_alive(bool)` - Keep alive option

### Serial Options
- `.device(const std::string&)` - Serial device path
- `.baud_rate(uint32_t)` - Baud rate
- `.data_bits(int)` - Data bits (5-8)
- `.stop_bits(int)` - Stop bits (1-2)
- `.parity(const std::string&)` - Parity ("none", "odd", "even")
- `.flow_control(const std::string&)` - Flow control ("none", "hardware", "software")
- `.timeout(std::chrono::milliseconds)` - Read/write timeout

## 📊 Comparison

| Feature | Old API | New API | Improvement |
|---------|---------|---------|-------------|
| **Initialization** | 5 steps | 1 step | 80% reduction |
| **Data Handling** | Manual byte conversion | Automatic string conversion | Much easier |
| **Lifecycle** | Manual start/stop | Automatic management | Simplified |
| **Configuration** | Limited | Extensive | More flexible |

## 🔄 Migration Guide

### From Old API to New API

**Old way:**
```cpp
TcpServerConfig cfg{port};
auto ul = unilink::create(cfg);
ul->on_bytes([&](const uint8_t* p, size_t n) {
    std::string s(reinterpret_cast<const char*>(p), n);
    // handle data
});
ul->start();
```

**New way:**
```cpp
auto server = unilink::tcp_server(port)
    .on_data([](const std::string& data) {
        // handle data
    })
    .auto_manage()
    .build();
```

## 🎯 Benefits

1. **Simplified API**: One-line server/client creation
2. **Type Safety**: Automatic string conversion
3. **Automatic Management**: RAII-based lifecycle
4. **Flexible Configuration**: Chain-based configuration
5. **Backward Compatibility**: Old API still works
