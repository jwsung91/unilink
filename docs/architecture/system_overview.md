# Unilink System Architecture

Comprehensive overview of unilink's architecture and design principles.

---

## Table of Contents

1. [Overview](#overview)
2. [Layered Architecture](#layered-architecture)
3. [Core Components](#core-components)
4. [Design Patterns](#design-patterns)
5. [Threading Model](#threading-model)
6. [Memory Management](#memory-management)
7. [Error Handling](#error-handling)

---

## Overview

Unilink is designed as a layered, modular communication library with clear separation of concerns. The architecture follows SOLID principles and employs modern C++ design patterns.

### Design Goals

- **Simplicity**: Easy-to-use Builder API
- **Safety**: Memory-safe, thread-safe operations
- **Performance**: Asynchronous I/O, efficient resource management
- **Flexibility**: Modular design, optional features
- **Reliability**: Comprehensive error handling and recovery

---

## Layered Architecture

```
┌─────────────────────────────────────────────────┐
│          User Application Layer                  │
├─────────────────────────────────────────────────┤
│          Builder API Layer                       │
│  (TcpClientBuilder, TcpServerBuilder, etc.)     │
├─────────────────────────────────────────────────┤
│          Wrapper API Layer                       │
│  (TcpClient, TcpServer, Serial)                 │
├─────────────────────────────────────────────────┤
│          Transport Layer                         │
│  (TcpTransport, SerialTransport)                │
├─────────────────────────────────────────────────┤
│          Common Utilities Layer                  │
│  (Thread Safety, Memory Management, Logging)    │
├─────────────────────────────────────────────────┤
│          Platform Layer (Boost.Asio)            │
└─────────────────────────────────────────────────┘
```

### Layer Responsibilities

#### 1. Builder API Layer
- **Purpose**: Provide fluent, chainable interface
- **Components**: `TcpClientBuilder`, `TcpServerBuilder`, `SerialBuilder`
- **Responsibilities**:
  - Configuration validation
  - Object construction
  - Callback registration

#### 2. Wrapper API Layer
- **Purpose**: High-level, easy-to-use interfaces
- **Components**: `TcpClient`, `TcpServer`, `Serial`
- **Responsibilities**:
  - Connection management
  - Data transmission
  - Lifecycle management
  - Event callbacks

#### 3. Transport Layer
- **Purpose**: Low-level communication implementation
- **Components**: Protocol-specific transports
- **Responsibilities**:
  - Actual I/O operations
  - Protocol handling
  - Connection state management

#### 4. Common Utilities Layer
- **Purpose**: Shared functionality
- **Components**: Thread safety, memory management, logging
- **Responsibilities**:
  - Thread-safe operations
  - Memory tracking
  - Error handling
  - Logging system

---

## Core Components

### 1. Builder System

```cpp
namespace unilink::builder {
    // Base interface
    template<typename T>
    class BuilderInterface { ... };
    
    // Concrete builders
    class TcpClientBuilder : public BuilderInterface<wrapper::TcpClient>;
    class TcpServerBuilder : public BuilderInterface<wrapper::TcpServer>;
    class SerialBuilder : public BuilderInterface<wrapper::Serial>;
}
```

**Key Features:**
- Method chaining
- Type-safe configuration
- Compile-time validation
- Auto-initialization support

### 2. Wrapper System

```cpp
namespace unilink::wrapper {
    // Base interface
    class ChannelInterface {
        virtual void send(const std::string& data) = 0;
        virtual void start() = 0;
        virtual void stop() = 0;
    };
    
    // Implementations
    class TcpClient : public ChannelInterface;
    class TcpServer : public ChannelInterface;
    class Serial : public ChannelInterface;
}
```

**Key Features:**
- Unified interface
- Automatic resource management
- Callback-based events
- Thread-safe operations

### 3. Transport System

```cpp
namespace unilink::transport {
    // TCP transport
    class TcpClient { ... };
    class TcpServer { ... };
    class TcpServerSession { ... };
    
    // Serial transport
    class Serial { ... };
}
```

**Key Features:**
- Boost.Asio based
- Asynchronous I/O
- Connection pooling (Planned)
- Retry logic

### 4. Common Utilities

```cpp
namespace unilink::common {
    // Thread safety
    template<typename T>
    class ThreadSafeState;
    
    template<typename T>
    class AtomicState;
    
    // Memory management
    class MemoryPool;
    class MemoryTracker;
    class SafeDataBuffer;
    
    // Logging
    class Logger;
    class LogRotation;
    
    // Error handling
    class ErrorHandler;
}
```

---

## Design Patterns

### 1. Builder Pattern

**Purpose**: Simplify object creation with many parameters

```cpp
// Instead of this:
TcpClient(host, port, retry_interval, callbacks...);

// We use this:
auto client = tcp_client(host, port)
    .retry_interval(3000)  // Optional, 3000ms is default
    .on_data(callback)
    .build();

client->start();  // Explicit lifecycle control
```

**Benefits:**
- Readable configuration
- Optional parameters
- Compile-time validation
- Method chaining

### 2. Dependency Injection

**Purpose**: Enable testability and flexibility

```cpp
// Interface-based design
class ISerialPort {
    virtual void open() = 0;
    virtual void close() = 0;
    virtual void async_read(...) = 0;
};

// Production implementation
class RealSerialPort : public ISerialPort { ... };

// Test implementation
class MockSerialPort : public ISerialPort { ... };

// Injection in constructor
Serial::Serial(std::shared_ptr<ISerialPort> port) : port_(port) { }
```

### 3. Observer Pattern

**Purpose**: Event notification system

```cpp
class TcpClient {
    std::function<void()> on_connect_;
    std::function<void(const std::string&)> on_data_;
    std::function<void()> on_disconnect_;
    
    void notify_connect() {
        if (on_connect_) on_connect_();
    }
    
    void notify_data(const std::string& data) {
        if (on_data_) on_data_(data);
    }
};
```

### 4. Singleton Pattern

**Purpose**: Global access to shared resources

```cpp
class Logger {
public:
    static Logger& instance() {
        static Logger instance;
        return instance;
    }
    
private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};
```

### 5. RAII (Resource Acquisition Is Initialization)

**Purpose**: Automatic resource management

```cpp
class TcpClient {
    ~TcpClient() {
        // Automatically stop and clean up
        stop();
    }
    
    std::unique_ptr<IoContext> io_context_;  // Auto-deleted
    std::shared_ptr<Connection> connection_; // Ref-counted
};
```

### 6. Template Method Pattern

**Purpose**: Define algorithm structure with customizable steps

```cpp
class ChannelInterface {
    void start() {
        before_start();
        do_start();      // Virtual - customizable
        after_start();
    }
    
protected:
    virtual void do_start() = 0;
    virtual void before_start() {}
    virtual void after_start() {}
};
```

---

## Threading Model

### Overview

```
┌─────────────────────────────────────────────┐
│         User Thread (Application)            │
│  - Calls API methods                         │
│  - Receives callbacks                        │
└──────────────────┬──────────────────────────┘
                   │
         ┌─────────▼──────────┐
         │  Thread-Safe Queue  │
         └─────────┬──────────┘
                   │
┌──────────────────▼──────────────────────────┐
│         I/O Thread (Boost.Asio)              │
│  - Handles network I/O                       │
│  - Manages connections                       │
│  - Executes async operations                 │
└─────────────────────────────────────────────┘
```

### Thread Safety Guarantees

#### 1. API Methods
All public API methods are thread-safe:

```cpp
// Safe to call from multiple threads
client->send("data1");  // Thread 1
client->send("data2");  // Thread 2
client->stop();         // Thread 3
```

#### 2. Callbacks
Callbacks are executed in the I/O thread context:

```cpp
.on_data([](const std::string& data) {
    // This runs in I/O thread
    // Don't block here!
});
```

#### 3. Shared State
All shared state is protected:

```cpp
class TcpClient {
    ThreadSafeState<State> state_;  // Thread-safe
    std::atomic<bool> connected_;   // Atomic
    std::mutex write_mutex_;        // Explicit lock
};
```

### IO Context Management

#### Shared Context (Default)
```cpp
// Multiple channels share one I/O thread
auto client1 = tcp_client("server1.com", 8080).build();
auto client2 = tcp_client("server2.com", 8080).build();
// Both use shared IoContextManager
```

**Pros:**
- Efficient resource usage
- Single I/O thread handles all connections

**Cons:**
- All connections share same thread

#### Independent Context
```cpp
// Each channel has its own I/O thread
auto client = tcp_client("server.com", 8080)
    .use_independent_context(true)
    .build();
```

**Pros:**
- Isolation between connections
- Useful for testing

**Cons:**
- More resource usage
- More threads

---

## Memory Management

### 1. Smart Pointers

```cpp
// Ownership
std::unique_ptr<TcpClient> client_;  // Exclusive ownership
std::shared_ptr<Session> session_;   // Shared ownership
std::weak_ptr<Connection> conn_;     // Non-owning reference
```

### 2. Memory Pool

For high-performance scenarios:

```cpp
class MemoryPool {
    std::vector<Block> blocks_;
    std::queue<Block*> free_list_;
    
public:
    void* allocate(size_t size);
    void deallocate(void* ptr);
};
```

**Use Case:**
```cpp
MemoryPool pool(1024, 100);  // 100 blocks of 1KB

// Allocate from pool (fast)
auto buffer = pool.allocate();
// ... use buffer ...
pool.deallocate(buffer);  // Return to pool
```

### 3. Memory Tracking

Debug memory leaks:

```cpp
#ifdef UNILINK_ENABLE_MEMORY_TRACKING
class MemoryTracker {
    void track_allocation(void* ptr, size_t size);
    void track_deallocation(void* ptr);
    void report_leaks();
};
#endif
```

### 4. Safe Data Buffer

Bounds-checked buffer:

```cpp
class SafeDataBuffer {
    std::vector<uint8_t> data_;
    
public:
    void append(const std::string& str);  // Bounds checked
    void append_bytes(const std::vector<uint8_t>& bytes);
    
    std::string to_string() const;  // Safe conversion
};
```

---

## Error Handling

### Error Propagation Flow

```
┌────────────────────────────────────────────┐
│  Transport Layer Error                      │
│  (Boost.Asio error_code)                   │
└─────────────────┬──────────────────────────┘
                  │
        ┌─────────▼──────────┐
        │  Error Handler       │
        │  - Categorize        │
        │  - Log               │
        │  - Notify callbacks  │
        └─────────┬──────────┘
                  │
   ┌──────────────┴──────────────┐
   │                             │
   ▼                             ▼
┌──────────────┐       ┌─────────────────┐
│ User Callback│       │ Retry Logic     │
│ (on_error)   │       │ (if applicable) │
└──────────────┘       └─────────────────┘
```

### Error Categories

```cpp
enum class ErrorCategory {
    NETWORK,        // Connection, timeout, etc.
    CONFIGURATION,  // Invalid config
    SYSTEM,         // OS errors
    MEMORY,         // Allocation failures
    VALIDATION      // Input validation
};

enum class ErrorLevel {
    INFO,           // Informational
    WARNING,        // Non-critical
    ERROR,          // Serious error
    CRITICAL        // Fatal error
};
```

### Error Recovery Strategies

#### 1. Automatic Retry
```cpp
class RetryPolicy {
    unsigned max_attempts{5};
    unsigned interval_ms{3000};
    bool exponential_backoff{false};
    
    bool should_retry(unsigned attempt);
    unsigned get_delay(unsigned attempt);
};
```

#### 2. Circuit Breaker (Planned)
```cpp
class CircuitBreaker {
    enum class State { CLOSED, OPEN, HALF_OPEN };
    
    State state_{State::CLOSED};
    unsigned failure_threshold_{5};
    std::chrono::seconds timeout_{30};
    
    bool allow_request();
    void record_success();
    void record_failure();
};
```

---

## Configuration System

### Compile-Time Configuration

```cpp
// CMake option: UNILINK_ENABLE_CONFIG
#ifdef UNILINK_ENABLE_CONFIG
namespace config {
    class ConfigManager { ... };
}
#endif

// CMake option: UNILINK_ENABLE_MEMORY_TRACKING
#ifdef UNILINK_ENABLE_MEMORY_TRACKING
class MemoryTracker { ... };
#endif
```

### Runtime Configuration

```cpp
struct TcpClientConfig {
    std::string host;
    uint16_t port;
    unsigned retry_interval_ms{3000};  // Default is 3 seconds
};

// Load from file
auto config = ConfigManager::load_from_file("config.json");
auto client_config = config->get_tcp_client_config("my_client");
```

---

## Performance Considerations

### 1. Asynchronous I/O
- Non-blocking operations
- Efficient event loop (Boost.Asio)
- Minimal context switching

### 2. Zero-Copy Operations
```cpp
// Avoid unnecessary copies
void send(std::string&& data);  // Move semantics
void send(std::string_view data);  // View (no copy)
```

### 3. Connection Pooling (Planned)
```cpp
// Reuse connections
class ConnectionPool {
    std::queue<Connection*> available_;
    std::vector<std::unique_ptr<Connection>> all_;
    
    Connection* acquire();
    void release(Connection* conn);
};
```

### 4. Memory Pooling
```cpp
// Avoid repeated allocations
MemoryPool buffer_pool_(1024, 100);
auto buffer = buffer_pool_.allocate();  // Fast!
```

---

## Extension Points

### 1. Custom Transports
```cpp
class MyCustomTransport : public transport::TransportInterface {
    void connect() override { /* ... */ }
    void send(const std::string& data) override { /* ... */ }
    // ... implement interface
};
```

### 2. Custom Builders
```cpp
class MyCustomBuilder : public BuilderInterface<MyWrapper> {
    std::unique_ptr<MyWrapper> build() override {
        return std::make_unique<MyWrapper>(/* ... */);
    }
};
```

### 3. Custom Error Handlers
```cpp
ErrorHandler::instance().register_callback([](const ErrorInfo& error) {
    // Custom error handling logic
    send_to_monitoring_system(error);
    if (error.level == ErrorLevel::CRITICAL) {
        trigger_alert();
    }
});
```

---

## Testing Architecture

### 1. Dependency Injection
```cpp
// Easy to mock
class Serial {
    Serial(std::shared_ptr<ISerialPort> port) : port_(port) {}
private:
    std::shared_ptr<ISerialPort> port_;
};

// In tests
auto mock_port = std::make_shared<MockSerialPort>();
Serial serial(mock_port);
```

### 2. Independent Contexts
```cpp
// Isolated testing
auto client = tcp_client("127.0.0.1", 8080)
    .use_independent_context(true)  // Own IO thread
    .build();
```

### 3. State Verification
```cpp
// Check internal state
ASSERT_EQ(client->get_state(), State::CONNECTED);
ASSERT_TRUE(server->is_listening());
```

---

## Summary

Unilink's architecture emphasizes:

✅ **Modularity** - Clear separation of concerns  
✅ **Safety** - Memory-safe, thread-safe operations  
✅ **Performance** - Asynchronous I/O, efficient resource management  
✅ **Testability** - Dependency injection, mockable interfaces  
✅ **Extensibility** - Easy to add new transports and features  
✅ **Maintainability** - Clean code, well-documented  

---

**See Also:**
- [Design Patterns](design_patterns.md)
- [Threading Model](threading_model.md)
- [Performance Guide](../guides/performance_tuning.md)

