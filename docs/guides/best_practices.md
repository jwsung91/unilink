# Unilink Best Practices Guide

Learn the recommended patterns and practices for using unilink effectively.

---

## Table of Contents

1. [Error Handling](#error-handling)
2. [Resource Management](#resource-management)
3. [Thread Safety](#thread-safety)
4. [Performance Optimization](#performance-optimization)
5. [Code Organization](#code-organization)
6. [Testing](#testing)
7. [Security](#security)
8. [Logging and Debugging](#logging-and-debugging)

---

## Error Handling

### ✅ DO: Always Register Error Callbacks

```cpp
// GOOD
auto client = unilink::tcp_client("server.com", 8080)
    .on_error([](const std::string& error) {
        log_error("TCP Client", error);
        // Handle error appropriately
    })
    .build();

// BAD - No error handling
auto client = unilink::tcp_client("server.com", 8080)
    .build();  // Errors will be silently ignored!
```

### ✅ DO: Check Connection Status Before Sending

```cpp
// GOOD
if (client->is_connected()) {
    client->send("Hello");
} else {
    log_warning("Cannot send - not connected");
}

// BAD - Send without checking
client->send("Hello");  // May fail silently
```

### ✅ DO: Implement Graceful Error Recovery

```cpp
// GOOD - Graceful recovery
auto client = unilink::tcp_client("server.com", 8080)
    .on_error([this](const std::string& error) {
        log_error(error);
        
        // Categorize error
        if (is_retryable(error)) {
            schedule_retry();
        } else {
            notify_user("Connection failed permanently");
        }
    })
    .retry_interval(5000)  // Auto-retry
    .build();

// BAD - No recovery strategy
auto client = unilink::tcp_client("server.com", 8080)
    .on_error([](const std::string& error) {
        std::cerr << error << std::endl;  // Just print and hope
    })
    .build();
```

### ✅ DO: Use Centralized Error Handler

```cpp
// GOOD - Centralized error management
void setup_error_handler() {
    unilink::common::ErrorHandler::instance()
        .register_callback([](const ErrorInfo& error) {
            // Log to file
            log_to_file(error);
            
            // Send to monitoring
            send_to_monitoring(error);
            
            // Alert on critical
            if (error.level == ErrorLevel::CRITICAL) {
                send_alert(error);
            }
        });
}

// Use in multiple components
auto client1 = tcp_client("server1.com", 8080).build();
auto client2 = tcp_client("server2.com", 8080).build();
// Both use the same error handler
```

---

## Resource Management

### ✅ DO: Use RAII and Smart Pointers

```cpp
// GOOD - Automatic cleanup
{
    auto client = unilink::tcp_client("server.com", 8080)
        .auto_manage(true)  // Auto cleanup
        .build();
    
    // Use client...
    
}  // client automatically cleaned up here

// BAD - Manual cleanup required
auto* client = new TcpClient(...);
// ... use client ...
client->stop();  // Must remember to call
delete client;   // Must remember to delete
```

### ✅ DO: Stop Connections Before Shutdown

```cpp
// GOOD - Graceful shutdown
class Application {
    std::shared_ptr<unilink::wrapper::TcpClient> client_;
    
    ~Application() {
        shutdown();
    }
    
    void shutdown() {
        if (client_) {
            client_->send("GOODBYE");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            client_->stop();
            client_.reset();
        }
    }
};

// BAD - Abrupt shutdown
class Application {
    std::shared_ptr<unilink::wrapper::TcpClient> client_;
    
    ~Application() {
        // client_ destroyed abruptly
    }
};
```

### ✅ DO: Reuse Connections When Possible

```cpp
// GOOD - Reuse connection
class DataSender {
    std::shared_ptr<unilink::wrapper::TcpClient> client_;
    
    void send_multiple_messages() {
        for (const auto& msg : messages) {
            if (client_->is_connected()) {
                client_->send(msg);
            }
        }
    }
};

// BAD - Create new connection each time
void send_message(const std::string& msg) {
    auto client = tcp_client("server.com", 8080).build();
    client->start();
    client->send(msg);
    client->stop();  // Wasteful!
}
```

---

## Thread Safety

### ✅ DO: Protect Shared State

```cpp
// GOOD - Thread-safe state management
class Server {
    std::mutex clients_mutex_;
    std::map<size_t, ClientInfo> clients_;
    
    void add_client(size_t id, const ClientInfo& info) {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_[id] = info;
    }
    
    size_t get_client_count() const {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        return clients_.size();
    }
};

// BAD - Race condition
class Server {
    std::map<size_t, ClientInfo> clients_;  // Not protected!
    
    void add_client(size_t id, const ClientInfo& info) {
        clients_[id] = info;  // UNSAFE from multiple threads!
    }
};
```

### ✅ DO: Use ThreadSafeState for Complex States

```cpp
// GOOD - Use provided thread-safe utilities
#include "unilink/common/thread_safe_state.hpp"

class Connection {
    unilink::common::ThreadSafeState<State> state_;
    
    void set_state(State new_state) {
        state_.set(new_state);  // Thread-safe
    }
    
    State get_state() const {
        return state_.get();  // Thread-safe
    }
};
```

### ❌ DON'T: Block in Callbacks

```cpp
// BAD - Blocking in callback
auto client = tcp_client("server.com", 8080)
    .on_data([](const std::string& data) {
        // This blocks the I/O thread!
        std::this_thread::sleep_for(std::chrono::seconds(10));
        process_data(data);
    })
    .build();

// GOOD - Process asynchronously
auto client = tcp_client("server.com", 8080)
    .on_data([this](const std::string& data) {
        // Queue for processing in another thread
        message_queue_.push(data);
    })
    .build();

// Separate processing thread
void processing_thread() {
    while (running_) {
        if (auto msg = message_queue_.pop()) {
            process_data(*msg);
        }
    }
}
```

---

## Performance Optimization

### ✅ DO: Use Move Semantics

```cpp
// GOOD - Move instead of copy
std::string large_data = generate_large_data();
client->send(std::move(large_data));  // Move, no copy

// BAD - Unnecessary copy
std::string large_data = generate_large_data();
client->send(large_data);  // Copy entire string
```

### ✅ DO: Enable Async Logging

```cpp
// GOOD - Async logging for performance
unilink::common::Logger::instance().enable_async(true);
unilink::common::Logger::instance().set_batch_size(100);

// Now logging is non-blocking
logger.info("component", "operation", "message");  // Fast!
```

### ✅ DO: Use Shared IO Context (Default)

```cpp
// GOOD - Shared context (efficient)
auto client1 = tcp_client("server1.com", 8080).build();
auto client2 = tcp_client("server2.com", 8080).build();
// Both share one I/O thread - efficient

// Only use independent context when needed for testing
auto test_client = tcp_client("test.com", 8080)
    .use_independent_context(true)  // Only for tests
    .build();
```

### ✅ DO: Batch Small Messages

```cpp
// GOOD - Batch small messages
class MessageBatcher {
    std::vector<std::string> batch_;
    const size_t max_batch_size_{100};
    
    void add_message(const std::string& msg) {
        batch_.push_back(msg);
        
        if (batch_.size() >= max_batch_size_) {
            flush();
        }
    }
    
    void flush() {
        std::string combined;
        for (const auto& msg : batch_) {
            combined += msg + "\n";
        }
        client_->send(combined);
        batch_.clear();
    }
};

// BAD - Send each message individually
for (const auto& msg : small_messages) {
    client->send(msg);  // Many small sends - inefficient
}
```

---

## Code Organization

### ✅ DO: Use Classes for Complex Logic

```cpp
// GOOD - Organized in a class
class ChatClient {
private:
    std::shared_ptr<unilink::wrapper::TcpClient> client_;
    std::string nickname_;
    
public:
    void connect(const std::string& server, uint16_t port) {
        client_ = unilink::tcp_client(server, port)
            .on_connect([this]() { handle_connect(); })
            .on_data([this](const std::string& data) { handle_data(data); })
            .on_disconnect([this]() { handle_disconnect(); })
            .build();
    }
    
private:
    void handle_connect() { /* ... */ }
    void handle_data(const std::string& data) { /* ... */ }
    void handle_disconnect() { /* ... */ }
};

// BAD - Everything in main()
int main() {
    auto client = tcp_client("server.com", 8080)
        .on_connect([]() { /* 100 lines of code */ })
        .on_data([](const std::string& data) { /* 200 lines */ })
        .build();
    // Hard to maintain!
}
```

### ✅ DO: Separate Concerns

```cpp
// GOOD - Separated concerns
class NetworkManager {
    void connect() { /* network logic */ }
    void disconnect() { /* network logic */ }
};

class MessageHandler {
    void process_message(const std::string& msg) { /* business logic */ }
};

class Application {
    NetworkManager network_;
    MessageHandler handler_;
    
    void start() {
        network_.on_data([this](const std::string& msg) {
            handler_.process_message(msg);  // Delegate
        });
    }
};

// BAD - Mixed concerns
class Application {
    void start() {
        client_->on_data([](const std::string& msg) {
            // Network logic
            // Business logic
            // UI logic
            // Database logic
            // All mixed together!
        });
    }
};
```

---

## Testing

### ✅ DO: Use Dependency Injection

```cpp
// GOOD - Testable design
class MessageProcessor {
    std::shared_ptr<IClient> client_;  // Interface
    
public:
    MessageProcessor(std::shared_ptr<IClient> client) 
        : client_(client) {}
    
    void send_message(const std::string& msg) {
        if (client_->is_connected()) {
            client_->send(msg);
        }
    }
};

// In production
auto real_client = tcp_client("server.com", 8080).build();
MessageProcessor processor(real_client);

// In tests
auto mock_client = std::make_shared<MockClient>();
MessageProcessor processor(mock_client);
EXPECT_CALL(*mock_client, send("test"));
```

### ✅ DO: Test Error Scenarios

```cpp
// GOOD - Test error handling
TEST(ClientTest, HandlesConnectionError) {
    bool error_received = false;
    
    auto client = tcp_client("invalid.server", 9999)
        .on_error([&](const std::string& error) {
            error_received = true;
        })
        .build();
    
    client->start();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    EXPECT_TRUE(error_received);
}
```

### ✅ DO: Use Independent Context for Tests

```cpp
// GOOD - Isolated tests
TEST(ClientTest, SendReceive) {
    auto client = tcp_client("127.0.0.1", test_port)
        .use_independent_context(true)  // Isolated I/O thread
        .build();
    
    // Test won't interfere with other tests
}
```

---

## Security

### ✅ DO: Validate All Input

```cpp
// GOOD - Input validation
void handle_message(const std::string& msg) {
    if (msg.empty()) {
        log_warning("Empty message received");
        return;
    }
    
    if (msg.size() > MAX_MESSAGE_SIZE) {
        log_warning("Message too large");
        return;
    }
    
    if (!is_valid_format(msg)) {
        log_warning("Invalid message format");
        return;
    }
    
    process_message(msg);
}

// BAD - No validation
void handle_message(const std::string& msg) {
    process_message(msg);  // Trusting input blindly
}
```

### ✅ DO: Implement Rate Limiting

```cpp
// GOOD - Rate limiting
class RateLimiter {
    std::map<size_t, std::deque<TimePoint>> request_times_;
    const size_t max_requests_per_second_{10};
    
    bool is_allowed(size_t client_id) {
        auto now = std::chrono::steady_clock::now();
        auto& times = request_times_[client_id];
        
        // Remove old entries
        while (!times.empty() && 
               now - times.front() > std::chrono::seconds(1)) {
            times.pop_front();
        }
        
        if (times.size() >= max_requests_per_second_) {
            return false;  // Rate limit exceeded
        }
        
        times.push_back(now);
        return true;
    }
};

// Use in server
server_->on_data([this, limiter](size_t id, const std::string& data) {
    if (!limiter->is_allowed(id)) {
        log_warning("Rate limit exceeded for client " + std::to_string(id));
        return;
    }
    
    process_data(id, data);
});
```

### ✅ DO: Set Connection Limits

```cpp
// GOOD - Limit connections
class Server {
    const size_t max_clients_{100};
    std::atomic<size_t> current_clients_{0};
    
    void on_connect(size_t id, const std::string& ip) {
        if (current_clients_ >= max_clients_) {
            log_warning("Max clients reached, rejecting " + ip);
            server_->send_to_client(id, "Server full\n");
            // Disconnect client
            return;
        }
        
        current_clients_++;
        // Accept connection
    }
};
```

---

## Logging and Debugging

### ✅ DO: Use Appropriate Log Levels

```cpp
// GOOD - Proper log levels
logger.debug("client", "send", "Sending data: " + data);  // Detailed
logger.info("client", "connect", "Connected to server");  // Important events
logger.warning("client", "retry", "Retrying connection");  // Potential issues
logger.error("client", "send", "Send failed: " + error);  // Errors
logger.critical("system", "init", "Fatal initialization error");  // Critical

// BAD - Wrong log levels
logger.info("client", "send", "Sending data: " + data);  // Too noisy
logger.error("client", "connect", "Connected to server");  // Not an error!
```

### ✅ DO: Enable Debug Logging During Development

```cpp
// GOOD - Debug mode for development
#ifdef DEBUG
    unilink::common::Logger::instance().set_level(LogLevel::DEBUG);
    unilink::common::ErrorHandler::instance().set_min_error_level(ErrorLevel::INFO);
#else
    unilink::common::Logger::instance().set_level(LogLevel::WARNING);
    unilink::common::ErrorHandler::instance().set_min_error_level(ErrorLevel::WARNING);
#endif
```

### ✅ DO: Log Context Information

```cpp
// GOOD - Rich context
logger.info("tcp_client", "connect", 
    "Connected to " + host + ":" + std::to_string(port) + 
    " (attempt " + std::to_string(attempt) + ")");

// BAD - Insufficient context
logger.info("tcp_client", "connect", "Connected");  // To what? When? How many attempts?
```

---

## Summary Checklist

### Error Handling
- [ ] Always register `.on_error()` callbacks
- [ ] Check connection status before sending
- [ ] Implement retry logic for recoverable errors
- [ ] Use centralized error handler

### Resource Management
- [ ] Use `.auto_manage(true)` when possible
- [ ] Call `stop()` before destruction
- [ ] Reuse connections instead of creating new ones
- [ ] Use RAII and smart pointers

### Thread Safety
- [ ] Protect all shared state with mutexes
- [ ] Don't block in callbacks
- [ ] Use `ThreadSafeState` for complex state
- [ ] Be aware of which thread executes callbacks

### Performance
- [ ] Use move semantics for large data
- [ ] Enable async logging
- [ ] Batch small messages
- [ ] Use shared IO context (default)

### Security
- [ ] Validate all input
- [ ] Implement rate limiting
- [ ] Set connection limits
- [ ] Sanitize log output

### Testing
- [ ] Use dependency injection
- [ ] Test error scenarios
- [ ] Use independent context for isolation
- [ ] Mock external dependencies

---

**See Also:**
- [API Guide](../reference/API_GUIDE.md)
- [Performance Tuning](performance_tuning.md)
- [Troubleshooting](troubleshooting.md)

