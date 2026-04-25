# Performance Guide {#user_performance}

This guide covers performance optimization strategies for `unilink`.

---

## Table of Contents

1. [Runtime Optimization](#runtime-optimization)
2. [Memory Optimization](#memory-optimization)
3. [Network Optimization](#network-optimization)

---

## Runtime Optimization

### 1. Threading Model & IO Context

**Use Shared IO Context (Default)**
Unilink defaults to a shared IO context model, which is highly efficient for most use cases.

```cpp
// ✅ GOOD: Shared context (efficient)
auto client1 = unilink::tcp_client("server1.com", 8080).build();
auto client2 = unilink::tcp_client("server2.com", 8080).build();
// All share ONE I/O thread - efficient!

// ❌ BAD: Independent contexts (wasteful)
auto client1 = unilink::tcp_client("server1.com", 8080)
    .independent_context(true)  // Creates dedicated thread
    .build();
```

### 2. Async Logging

Logging can be a major bottleneck. Enable async logging for high-performance applications.

```cpp
// ✅ GOOD: Async logging (non-blocking)
unilink::diagnostics::AsyncLogConfig config;
config.batch_size = 1000;
config.flush_interval = std::chrono::milliseconds(1000);

unilink::diagnostics::Logger::instance().set_async_logging(true, config);
```

### 3. Non-Blocking Callbacks

Never perform heavy computation or blocking operations (like `sleep`) inside callbacks.

```cpp
// ❌ BAD: Blocking I/O thread
.on_data([](const unilink::MessageContext& ctx) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    process_data(ctx.data());
})

// ✅ GOOD: Offload to worker thread/pool
.on_data([&thread_pool](const unilink::MessageContext& ctx) {
    std::string payload(ctx.data());
    thread_pool.submit([payload = std::move(payload)]() {
        process_data(payload);
    });
})
```

---

## Memory Optimization

### 1. Avoid Data Copies
Use move semantics and avoid unnecessary string copies.

```cpp
// ✅ GOOD: Move (no copy)
std::string large_data = generate_data(1024 * 1024);
client->send(std::move(large_data));

// ✅ GOOD: Use string_view for parsing (if supported)
void parse(std::string_view msg) { ... }
```

### 2. Reserve Vector Capacity
When building vectors of data, always `reserve` capacity to avoid reallocations.

```cpp
std::vector<std::string> messages;
messages.reserve(1000);  // Pre-allocate
for (int i = 0; i < 1000; ++i) messages.push_back(msg);
```

---

## Network Optimization

### 1. Batch Small Messages
Sending many small packets incurs high system call overhead.

```cpp
// ❌ BAD: 1000 system calls
for (int i = 0; i < 1000; ++i) client->send("msg");

// ✅ GOOD: Batch into single send
std::string batch;
batch.reserve(4000);
for (int i = 0; i < 1000; ++i) batch += "msg";
client->send(batch);
```

### 2. Connection Reuse
Reusing connections avoids repeated TCP handshake and connection setup overhead. For workloads with many short requests to the same peer, create the client once and reuse it across calls.

### 3. Socket Tuning
For extremely high throughput, tune OS-level socket buffers:
```bash
sudo sysctl -w net.core.rmem_max=16777216  # 16 MB
sudo sysctl -w net.core.wmem_max=16777216  # 16 MB
```


