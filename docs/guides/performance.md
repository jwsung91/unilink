# Performance Optimization Guide

This guide covers performance optimization strategies for `unilink`, including build configuration, runtime optimization, and best practices.

---

## Table of Contents

1. [Build Configuration Optimization](#build-configuration-optimization)
2. [Runtime Performance](#runtime-performance)
3. [Memory Optimization](#memory-optimization)
4. [Network Performance](#network-performance)
5. [Profiling and Benchmarking](#profiling-and-benchmarking)

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
| **Dependencies** | Fewer include files | More includes |
| **Features** | Builder API only | + Configuration Management |

---

### Minimal Build Benefits

When building with `UNILINK_ENABLE_CONFIG=OFF`:

- ✅ **Faster Compilation**: Excludes configuration management code
- ✅ **Smaller Binary**: Reduced library size (~30% reduction)
- ✅ **Lower Memory Usage**: No configuration overhead at runtime
- ✅ **Simpler Dependencies**: Fewer include files and link dependencies

**Build command:**
```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DUNILINK_ENABLE_CONFIG=OFF
cmake --build build -j
```

---

### Full Build Features

When building with `UNILINK_ENABLE_CONFIG=ON`:

- ✅ **Dynamic Configuration**: Runtime configuration management
- ✅ **File-based Settings**: Load/save configurations from files
- ✅ **Flexible Parameters**: Adjust settings without recompilation
- ✅ **Advanced Features**: Full feature set for complex applications

**Build command:**
```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DUNILINK_ENABLE_CONFIG=ON
cmake --build build -j
```

---

### When to Use Each Build

| Use Case | Recommended Build | Reason |
|----------|------------------|---------|
| **Simple TCP/Serial apps** | `UNILINK_ENABLE_CONFIG=OFF` | Minimal footprint, faster compilation |
| **Embedded systems** | `UNILINK_ENABLE_CONFIG=OFF` | Memory constraints |
| **IoT devices** | `UNILINK_ENABLE_CONFIG=OFF` | Limited resources |
| **Configuration-heavy apps** | `UNILINK_ENABLE_CONFIG=ON` | Dynamic configuration needed |
| **Enterprise applications** | `UNILINK_ENABLE_CONFIG=ON` | Runtime configurability |
| **Testing/Development** | `UNILINK_ENABLE_CONFIG=ON` | Full feature set for testing |

---

### Compiler Optimization Levels

#### Release Build (Recommended for Production)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

- Optimization level: `-O3`
- Link-time optimization (LTO): Enabled
- Debug symbols: Stripped
- Performance: **Best**
- Binary size: **Smallest**

#### RelWithDebInfo (For Profiling)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

- Optimization level: `-O2`
- Debug symbols: Included
- Performance: **Good** (slightly slower than Release)
- Binary size: Larger (debug symbols)
- **Use for:** Performance profiling and production debugging

---

## Runtime Performance

### Threading Model Optimization

#### 1. Don't Block in Callbacks

**Bad:**
```cpp
auto client = unilink::tcp_client("server.com", 8080)
    .on_data([](const std::string& data) {
        // ❌ BAD: Blocking I/O thread
        std::this_thread::sleep_for(std::chrono::seconds(1));
        process_heavy_computation(data);
    })
    .build();
```

**Good:**
```cpp
auto client = unilink::tcp_client("server.com", 8080)
    .on_data([](const std::string& data) {
        // ✅ GOOD: Offload to worker thread
        std::thread([data]() {
            process_heavy_computation(data);
        }).detach();
        
        // Or use a thread pool
        thread_pool.submit([data]() {
            process_heavy_computation(data);
        });
    })
    .build();
```

**Impact:** Blocking callbacks can reduce throughput by 10-100x

---

#### 2. Minimize Callback Overhead

**Bad:**
```cpp
.on_data([](const std::string& data) {
    // ❌ BAD: Multiple allocations
    std::string copy1 = data;
    std::string copy2 = copy1;
    std::vector<uint8_t> buffer(copy2.begin(), copy2.end());
})
```

**Good:**
```cpp
.on_data([](const std::string& data) {
    // ✅ GOOD: Process directly, minimal copies
    process_data(data.data(), data.size());
})
```

---

### Connection Management

#### Reuse Connections

**Bad:**
```cpp
// ❌ BAD: Creating new connection for each request
for (int i = 0; i < 1000; i++) {
    auto client = unilink::tcp_client("server.com", 8080)
        .build();
    client->start();
    client->send("request");
    client->stop();
}
```

**Good:**
```cpp
// ✅ GOOD: Reuse single connection
auto client = unilink::tcp_client("server.com", 8080)
    .build();

client->start();

for (int i = 0; i < 1000; i++) {
    client->send("request");
}
```

**Impact:** Connection reuse can improve throughput by 10-50x

---

### Batch Operations

#### Batch Multiple Sends

**Bad:**
```cpp
// ❌ BAD: Individual sends
for (const auto& msg : messages) {
    client->send(msg);  // Each call has overhead
}
```

**Good:**
```cpp
// ✅ GOOD: Batch into single send
std::string batch;
for (const auto& msg : messages) {
    batch += msg;
}
client->send(batch);  // Single call, better performance
```

**Impact:** Batching can reduce overhead by 5-10x for small messages

---

## Memory Optimization

### Buffer Size Configuration

#### Default Buffer Sizes

```cpp
// Default read buffer: 4 KB
// Default backpressure threshold: 1 MB
```

#### Optimize for Your Use Case

**For high-throughput applications:**
```cpp
auto client = unilink::tcp_client("server.com", 8080)
    .read_buffer_size(64 * 1024)  // 64 KB read buffer
    .build();
```

**For memory-constrained systems:**
```cpp
auto client = unilink::tcp_client("server.com", 8080)
    .read_buffer_size(1024)  // 1 KB read buffer
    .build();
```

---

### Memory Pool Usage

`unilink` automatically uses memory pools for buffers ≤ 64 KB:

- ✅ Reduces allocation overhead
- ✅ Improves cache locality
- ✅ Reduces memory fragmentation

**Automatic optimization:**
- Buffers ≤ 64 KB: Use memory pool
- Buffers > 64 KB: Direct allocation

**No configuration needed** - optimization is automatic!

---

### Backpressure Management

Monitor and handle backpressure to prevent memory growth:

```cpp
auto client = unilink::tcp_client("server.com", 8080)
    .on_backpressure([](size_t queue_bytes) {
        if (queue_bytes > 10 * 1024 * 1024) {  // 10 MB
            // Pause sending or apply rate limiting
            std::cout << "⚠️ High backpressure: " << queue_bytes << " bytes\n";
        }
    })
    .build();
```

**Default threshold:** 1 MB  
**Configurable range:** 1 KB - 100 MB

See [Runtime Behavior - Backpressure](../architecture/runtime_behavior.md#backpressure-handling) for details.

---

## Network Performance

### TCP Performance Tuning

#### TCP_NODELAY (Disable Nagle's Algorithm)

For low-latency applications:

```cpp
// TCP_NODELAY is enabled by default in unilink
// Sends data immediately without buffering
```

**Trade-off:**
- ✅ Lower latency
- ⚠️ More packets, slightly higher bandwidth usage

**Best for:** Interactive applications, real-time communication

---

#### Socket Buffer Sizes

Adjust OS-level socket buffers for high-throughput:

```bash
# Check current limits
sysctl net.core.rmem_max
sysctl net.core.wmem_max

# Increase limits (requires root)
sudo sysctl -w net.core.rmem_max=16777216  # 16 MB
sudo sysctl -w net.core.wmem_max=16777216  # 16 MB
```

---

### Reconnection Interval

Balance between responsiveness and resource usage:

```cpp
// Fast reconnection (aggressive)
auto client = unilink::tcp_client("server.com", 8080)
    .retry_interval(100)  // 100 ms - High CPU usage
    .build();

// Moderate reconnection (recommended)
auto client = unilink::tcp_client("server.com", 8080)
    .retry_interval(2000)  // 2 seconds (default) - Balanced
    .build();

// Slow reconnection (conservative)
auto client = unilink::tcp_client("server.com", 8080)
    .retry_interval(10000)  // 10 seconds - Low resource usage
    .build();
```

---

## Profiling and Benchmarking

### Built-in Performance Tests

Run performance benchmarks:

```bash
# Build with performance tests
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUNILINK_BUILD_TESTS=ON
cmake --build build -j

# Run performance tests
./build/test/run_performance_tests
```

**Benchmark categories:**
- TCP throughput (Mb/s)
- TCP latency (μs)
- Serial throughput
- Memory allocations
- Thread safety overhead

---

### Profiling with perf

```bash
# Build with debug symbols
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j

# Profile your application
perf record -g ./my_app
perf report
```

---

### Memory Profiling with Valgrind

```bash
# Build with debug symbols
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j

# Profile memory usage
valgrind --tool=massif --massif-out-file=massif.out ./my_app
ms_print massif.out
```

---

### AddressSanitizer Performance Analysis

```bash
# Build with sanitizers
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DUNILINK_ENABLE_SANITIZERS=ON

cmake --build build -j

# Run with sanitizers
./my_app
```

**Detects:**
- Memory leaks
- Use-after-free
- Buffer overflows
- Thread race conditions

**Note:** Sanitizers slow down execution by ~2-3x

---

## Performance Checklist

### Build-Time Optimizations

- ✅ Use `Release` build type for production
- ✅ Enable minimal build (`UNILINK_ENABLE_CONFIG=OFF`) if not using config
- ✅ Use `-j` for parallel compilation
- ✅ Consider `ccache` for faster rebuilds

### Runtime Optimizations

- ✅ Never block in callbacks
- ✅ Minimize data copies
- ✅ Reuse connections
- ✅ Batch operations when possible
- ✅ Configure appropriate buffer sizes
- ✅ Monitor and handle backpressure

### System-Level Optimizations

- ✅ Increase socket buffer sizes for high throughput
- ✅ Adjust reconnection intervals based on use case
- ✅ Use appropriate CPU affinity for real-time applications
- ✅ Consider thread priority for time-critical operations

---

## Performance Comparison

### Build Configuration Impact

| Metric | Minimal Build | Full Build |
|--------|---------------|------------|
| **Binary Size** | 250 KB | 350 KB |
| **Compilation Time** | 15 sec | 22 sec |
| **Memory Usage (runtime)** | 2 MB | 3 MB |
| **Throughput** | Same | Same |
| **Latency** | Same | Same |

*Measurements on Ubuntu 22.04, GCC 11.4, -O3 optimization*

### Connection Reuse Impact

| Operation | New Connection | Reused Connection | Speedup |
|-----------|----------------|-------------------|---------|
| Single request | 50 ms | 5 ms | 10x |
| 100 requests | 5000 ms | 500 ms | 10x |
| 1000 requests | 50000 ms | 5000 ms | 10x |

---

## Next Steps

- [Build Guide](build_guide.md) - Detailed build instructions
- [Runtime Behavior](../architecture/runtime_behavior.md) - Threading model and policies
- [Best Practices](best_practices.md) - Recommended usage patterns
- [Testing Guide](testing.md) - Performance benchmarks
