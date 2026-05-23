# Performance Guide {#user_performance}

This guide covers performance optimization strategies for `unilink`.

---

## Table of Contents

1. Runtime Optimization
2. Memory Optimization
3. Network Optimization

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
sudo sysctl -w net.core.rmem_max=16777216  # 16 MB (OS limit)
sudo sysctl -w net.core.wmem_max=16777216  # 16 MB (OS limit)
```

---

## Backpressure Management {#backpressure-management}

When the data generation rate exceeds the network's transmission capacity, Unilink's internal send queues grow. Managing this "backpressure" is critical for stability and latency.

### 1. Choosing a Strategy

Unilink provides two strategies for handling full queues:

| Strategy | Behavior | Best For |
|:---|:---|:---|
| `Reliable` (Default) | Preserves queued outgoing data until the configured threshold is reached (default: 1 MiB), bounded by internal queue limits. | Reliable data (files, logs, commands). |
| `BestEffort` | Drops oldest data when a threshold is reached. | Real-time sensors (LiDAR, Video, Telemetry). |

### Interpreting Strategy Results

For throughput or pressure tests, distinguish between:

- **accepted throughput**: data accepted into the local send path
- **received throughput**: data observed by the receiver
- **delivery rate**: received data relative to accepted data
- **failed sends**: send calls that were rejected before being accepted
- **dropped data**: queued data discarded by a freshness-oriented policy such as `BestEffort`

In `BestEffort` mode, accepted throughput can be much higher than received throughput because older queued payloads may be dropped to keep newer data fresh. This is expected for unbounded producer pressure, but it must be monitored in real applications.

In `Reliable` mode, throughput is usually limited by queue pressure and receiver progress, but accepted data is preserved unless the queue cannot accept more data.

Use `stats()` to monitor queue pressure and drops:

- `queued_bytes`
- `pending_bytes`
- `max_queued_bytes`
- `dropped_messages`
- `dropped_bytes`
- `failed_sends`
- `backpressure_events`

### 2. High-Throughput Sensors (LiDAR/Camera)

For robotics perception, processing stale data is often worse than skipping frames. Use `BestEffort` with a low threshold to prevent **Bufferbloat**.

```cpp
// 🤖 Best configuration for robotics sensors
auto lidar = unilink::tcp_client("192.168.1.10", 2368)
    .backpressure_strategy(unilink::base::constants::BackpressureStrategy::BestEffort)
    .backpressure_threshold(1024 * 512)  // 0.5 MB threshold
    .build();
```

Python-specific performance notes are maintained in
[unilink-lab/unilink-python](https://github.com/unilink-lab/unilink-python).

Aggressive flushing during network disconnects ensures that once reconnection
occurs, the receiver instantly gets the most recent frame instead of waiting for
a large backlog to drain.

For freshness-oriented streams, monitor dropped data and received rate. A high accepted rate alone does not mean the receiver is processing every payload.

### 3. Critical Reliable Data

For data where local queue drops are unacceptable, use `Reliable` combined with application-level send throttling.

For producer loops that must never block, use `try_send()` and handle `false` explicitly. `send()` in `Reliable` mode may block while waiting for backpressure to clear.

```cpp
bool paused = false;
client->on_backpressure([&paused](size_t queued) {
    // Pause/resume based on your application's queued-byte budget.
    if (queued > 12 * 1024 * 1024) paused = true;
    else if (queued < 4 * 1024 * 1024) paused = false;
});

// In your sender loop
if (!paused) client->send(critical_data);
```
