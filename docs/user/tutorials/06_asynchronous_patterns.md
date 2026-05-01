# Asynchronous Programming Patterns {#user_tutorial_async}

Unilink is designed as an asynchronous, event-driven library from the ground up. While `start_sync()` is available for convenience, mastering asynchronous patterns is key to building high-performance applications.

---

## 1. Non-Blocking Startup

When you call `start()`, the library initiates the connection process (for clients) or port binding (for servers) in a background thread.

### The Async Pattern

```cpp
auto client = unilink::tcp_client("127.0.0.1", 8080)
    .on_connect([](const unilink::ConnectionContext& ctx) {
        std::cout << "Connected! This fires from a background thread." << std::endl;
    })
    .build();

// start() returns immediately
client->start(); 

// You can do other initialization here
std::cout << "Waiting for connection in background..." << std::endl;
```

---

## 2. Shared Ownership in Callbacks

Since callbacks fire from background threads at unpredictable times, you must ensure that the communication objects (like `client` or `server`) remain alive.

### Safe Capture Pattern

The recommended way is to use `std::shared_ptr` and capture it by value or reference in the callback.

```cpp
// 1. Declare a shared pointer
std::shared_ptr<unilink::wrapper::TcpClient> client;

// 2. Capture the pointer in the builder (using a reference to the shared_ptr variable)
auto builder = unilink::tcp_client("127.0.0.1", 8080)
    .on_connect([&client](const unilink::ConnectionContext&) {
        // Safe to use client here because it's captured
        client->send("System Online");
    });

// 3. Move the built object into the shared pointer
client = builder.build();
client->start();
```

---

## 3. Parallel Initialization

Asynchronous startup allows you to initialize multiple connections simultaneously without waiting for each one to complete sequentially.

```cpp
auto sensor1 = unilink::serial("/dev/ttyUSB0", 115200).build();
auto sensor2 = unilink::serial("/dev/ttyUSB1", 115200).build();
auto cloud_link = unilink::tcp_client("api.mycloud.com", 80).build();

// Start all three at once
sensor1->start();
sensor2->start();
cloud_link->start();

// None of the above blocked. The main thread can proceed to 
// setup the UI or other logic immediately.
```

---

## 4. When to Use Async vs Sync

| Feature | `start_sync()` | `start()` (Async) |
|---------|----------------|-------------------|
| **Blocking** | Yes, until connected | No, immediate return |
| **Error Handling** | Return value (`bool`) | `on_error` callback |
| **Complexity** | Low | Moderate (requires callbacks) |
| **Best For** | CLI tools, simple scripts | GUI apps, High-throughput servers, Multi-channel systems |

---

## Summary

- Use `start()` for non-blocking execution.
- Register all event handlers (`on_data`, `on_connect`, etc.) **before** calling `start()`.
- Be mindful of object lifetimes in lambda captures.
- Prefer asynchronous patterns for production systems to maximize responsiveness.
