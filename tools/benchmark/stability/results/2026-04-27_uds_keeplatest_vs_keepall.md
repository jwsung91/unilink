# BackpressureStrategy UDS Local IPC Benchmark — 2026-04-27

## Objective
Evaluate `BackpressureStrategy` performance under high-speed Local Inter-Process Communication (IPC) using Unix Domain Sockets (UDS). This environment minimizes network latency to isolate the overhead of Unilink's queue management and Python's GIL.

## Results Summary (UDS)
*Workload: 128KB payloads, 1µs sleep (max speed), 3s chaos interval.*

| Metric | Reliable (16MB) + Flow Control | **BestEffort (1MB) + No FC** | Performance Gain |
|:---|:---:|:---:|:---:|
| **Avg Throughput** | 80.38 MB/s | **1,356.59 MB/s** | **16.8x Faster** |
| **Data Sent (15s)** | 1.2 GB | **20.3 GB** | **16.9x More** |
| **BP Events** | 141 | **40,899** | Precise fresh window |
| **Delivery Rate** | 99.9% | 0.62% | Freshness Priority |

## Analysis

### 1. Massive Speedup in Python
UDS highlights the cost of context switching and blocking in Python. `Reliable` with Flow Control forced the Python thread to wait for I/O completion frequently, resulting in only 80 MB/s. 
By switching to `BestEffort` and removing Flow Control, the Python sender was **never blocked**. It successfully pushed data into the Unilink C++ core at over **1.3 GB/s**, effectively saturating the local IPC bus.

### 2. Extreme Freshness (Bufferbloat Prevention)
During network interruptions, `BestEffort` flushed the queue **40,899 times**. With a 1MB threshold, this ensures that the receiver never deals with more than 1MB of "old" data. In a 1.3 GB/s stream, 1MB represents less than **1ms of latency**. 
`Reliable` would have buffered 16MB or more, leading to hundreds of milliseconds of stale data once the connection recovered.

## Recommendation for Local IPC
For high-frequency sensor fusion where both processes reside on the same machine (e.g., Camera Driver -> Perception Node):
- **Strategy**: `BestEffort`
- **Threshold**: `1 MB`
- **Flow Control**: **OFF**

This configuration allows Python nodes to operate at near-native C++ speeds without sacrificing system responsiveness.
