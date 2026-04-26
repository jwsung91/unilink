# Benchmark Results {#user_benchmark}

Latest benchmark results for Unilink v1.1 (Stability Patch applied).
Tested on: Ubuntu 24.04, Python 3.12, pybind11 2.13.6.

## 1. TCP Performance (RTT & Throughput)

### Ping-Pong (Latency)
Measured as round-trip time (RTT) for small messages.

| Messages | Python (Native) | Unilink (Python) | Unilink (C++) |
| :--- | :--- | :--- | :--- |
| 100 | 0.003s | 0.007s | < 0.001s |
| 500 | 0.015s | 0.028s | 0.002s |
| 1000 | 0.030s | 0.058s | 0.004s |

*Note: Python overhead is primarily due to GIL acquisition and async dispatch (~50µs per call).*

### Throughput (1KB Chunks)
Streaming data performance.

| Chunks | Python (Native) | Unilink (Direct) | Unilink (Batching) |
| :--- | :--- | :--- | :--- |
| 1000 | 0.0017s | 0.0035s | 0.0025s |
| 5000 | 0.0070s | 0.0177s | 0.0075s |
| 10000 | 0.0127s | 0.0381s | 0.0186s |

*Stability Note: With the v1.1 patch, Unilink handles 10,000+ chunks without connection drops by using intelligent backpressure message dropping.*

## 2. UDP & UDS Performance

### UDP Latency (Zero-Copy)
| Messages | Python | Unilink | Unilink (ZeroCopy) |
| :--- | :--- | :--- | :--- |
| 1000 | 0.031s | 0.062s | 0.060s |

### UDS Throughput
| Chunks | Python | Unilink |
| :--- | :--- | :--- |
| 1000 | 0.056s | 0.054s |

## 3. Optimization Recommendations

1. **Batching**: Use `batch_size()` and `batch_latency()` for streaming small data.
2. **Backpressure**: Increase `DEFAULT_BACKPRESSURE_THRESHOLD` (now 16MB) if dropping too many messages.
3. **GIL Management**: Unilink now releases GIL during blocking `stop()` and `start()` operations to prevent deadlocks.
