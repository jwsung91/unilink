# Performance Guide

This guide covers performance optimization strategies for `unilink`, including build configuration, runtime optimization, tuning techniques, and benchmarking.

---

## Table of Contents

1. [Performance Overview](#performance-overview)
2. [Build Configuration Optimization](#build-configuration-optimization)
3. [Runtime Optimization](#runtime-optimization)
4. [Memory Optimization](#memory-optimization)
5. [Network Optimization](#network-optimization)
6. [Benchmarking & Profiling](#benchmarking--profiling)
7. [Real-World Case Studies](#real-world-case-studies)

---

## Performance Overview

### Characteristics & Goals

| Metric | Typical Value | Excellent Value |
|--------|---------------|-----------------|
| Connection Time | < 100ms | < 10ms (local) |
| Latency | < 1ms (local) | < 0.1ms |
| Throughput | > 100 MB/s | > 1 GB/s (local) |
| CPU Usage | < 5% | < 1% (idle) |
| Memory / Connection | < 10 KB | < 5 KB |

**Goals:**
- **Low Latency**: Minimize time from send to receive.
- **High Throughput**: Maximize data transfer rate.
- **Low Resource Usage**: Efficient CPU and memory usage.
- **Scalability**: Handle many concurrent connections.

---

## Build Configuration Optimization

### Minimal Build vs Full Build

Choose the right build configuration based on your use case:

| Feature | Minimal Build | Full Build |
|---------|---------------|------------|
| **Configuration** | `UNILINK_ENABLE_CONFIG=OFF` | `UNILINK_ENABLE_CONFIG=ON` |
| **Binary Size** | Smaller (~30% reduction) | Larger |
| **Compilation Time** | Faster | Slower |
| **Memory Usage** | Lower (no config overhead) | Higher |
| **Features** | Builder API only | + Configuration Management |

**Build command (Minimal):**
```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DUNILINK_ENABLE_CONFIG=OFF
cmake --build build -j
```

### Compiler Optimization Levels

Always use **Release** builds for production performance.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

- **Flags**: `-O3` (Highest optimization), `-flto` (Link-time optimization)
- **Impact**: 5-15% performance improvement compared to default release builds.

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
    .use_independent_context(true)  // Creates dedicated thread
    .build();
```

**Impact**: Reduces memory usage by ~90% per connection and CPU context switching by ~80%.

### 2. Async Logging

Logging can be a major bottleneck. Enable async logging for high-performance applications.

```cpp
// ✅ GOOD: Async logging (non-blocking)
unilink::common::AsyncLogConfig config;
config.batch_size = 1000;
config.flush_interval = std::chrono::milliseconds(1000);

unilink::common::Logger::instance().set_async_logging(true, config);
```

**Impact**: 10-100x faster logging throughput.

### 3. Non-Blocking Callbacks

Never perform heavy computation or blocking operations (like `sleep`) inside callbacks.

```cpp
// ❌ BAD: Blocking I/O thread
.on_data([](const std::string& data) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    process_data(data);
})

// ✅ GOOD: Offload to worker thread/pool
.on_data([&thread_pool](const std::string& data) {
    thread_pool.submit([data]() {
        process_data(data);
    });
})
```

---

## Memory Optimization

### 1. Memory Pool Usage
`unilink` automatically uses memory pools for buffers ≤ 64 KB.
- **Buffers ≤ 64 KB**: Use memory pool (fast allocation, no fragmentation).
- **Buffers > 64 KB**: Direct allocation.

### 2. Avoid Data Copies
Use move semantics and avoid unnecessary string copies.

```cpp
// ✅ GOOD: Move (no copy)
std::string large_data = generate_data(1024 * 1024);
client->send(std::move(large_data));

// ✅ GOOD: Use string_view for parsing (if supported)
void parse(std::string_view msg) { ... }
```

### 3. Reserve Vector Capacity
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

**Impact**: up to 50x throughput improvement for small messages.

### 2. Connection Reuse
Reusing connections is significantly faster than creating new ones.

| Operation | New Connection | Reused Connection | Speedup |
|-----------|----------------|-------------------|---------|
| Single request | 50 ms | 5 ms | 10x |

### 3. Socket Tuning
For extremely high throughput, tune OS-level socket buffers:
```bash
sudo sysctl -w net.core.rmem_max=16777216  # 16 MB
sudo sysctl -w net.core.wmem_max=16777216  # 16 MB
```

---

## Benchmarking & Profiling

### Built-in Performance Tests
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUNILINK_BUILD_TESTS=ON
cmake --build build -j
./build/test/run_performance_tests
```

### Profiling with Tools

**Linux `perf`:**
```bash
# Build with debug symbols
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
perf record -g ./my_app
perf report
```

**Valgrind (Memory):**
```bash
valgrind --tool=massif ./my_app
ms_print massif.out
```

### Simple Throughput Benchmark Code
```cpp
void benchmark_throughput() {
    auto client = unilink::tcp_client("127.0.0.1", 8080).build();
    client->start();
    while (!client->is_connected()) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    std::string data(1024, 'X'); // 1KB
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < 10000; ++i) client->send(data);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto seconds = std::chrono::duration<double>(end - start).count();
    std::cout << "Throughput: " << (10000 * 1024 / 1024.0 / 1024.0) / seconds << " MB/s\n";
}
```

---

## Real-World Case Studies

### Case Study 1: High-Throughput Data Streaming
**Scenario**: Stream 1 GB/s of data from sensors.
**Optimizations**: Binary protocol, 1000-sample batching, memory pools, async logging.
**Result**: **10x improvement** (120 MB/s -> 1.2 GB/s), CPU usage dropped from 80% to 15%.

### Case Study 2: Low-Latency Trading System
**Scenario**: < 100μs latency required.
**Optimizations**: Pinned threads, lock-free queues, pre-allocated buffers, disabled logging.
**Result**: **6x improvement** (450μs -> 75μs avg latency).

### Case Study 3: IoT Gateway (1000+ Connections)
**Scenario**: Handle 1000 concurrent sensor connections.
**Optimizations**: Shared IO context, connection pooling, rate limiting.
**Result**: **2.4x capacity increase**, 60% less memory usage (2GB -> 800MB).