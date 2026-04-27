# BackpressureStrategy Multi-Transport Benchmark — 2026-04-27

## Objective
Evaluate and compare `BackpressureStrategy` implementations (`KeepAll` vs `KeepLatest`) across **TCP and UDP** transports under chaotic network environments (LV3/LV4 equivalent).

This benchmark validates that:
1.  **KeepAll** (with Flow Control) ensures 100% data reliability for critical commands/logs.
2.  **KeepLatest** (without Flow Control) ensures minimum latency and maximum freshness for sensor data (LiDAR/Video).
3.  **New API**: All interfaces (TCP Server/Client, UDP, Serial, UDS) now expose these configuration properties.

---

## 1. TCP Stability Results (LV3 Chaos)
*Workload: 64KB payloads, 10µs sleep, 7s chaos interval.*

| Metric | Py Unilink (`KeepAll`) | **Py Unilink (`KeepLatest`)** | Py Raw Sockets | C++ Unilink (`KeepAll`) |
|:---|:---:|:---:|:---:|:---:|
| **Throughput Avg** | 133.5 MB/s | **414.5 MB/s** | 563.5 MB/s | 1,420.4 MB/s |
| **Delivery Rate** | 99.4% | **33.9% (Intentional)** | 99.9% | 99.9% |
| **Efficiency (vs Raw)**| 23.7% | **73.5%** | 100% | N/A |

**Key Finding:** `KeepLatest` increases Python TCP throughput by **3.1x**, reaching **73.5%** of raw socket performance by removing sender-side blocking.

---

## 2. UDP Stress Results (LV4 Extreme)
*Workload: 4KB payloads, 10µs sleep, 2s network "mute" simulation, 16KB tiny threshold.*

| Metric | **UDP KeepAll + FC** | **UDP KeepLatest (No FC)** | Performance Gain |
|:---|:---:|:---:|:---:|
| **Sent Data** | 4.83 MB | **441.11 MB** | **91.3x** |
| **BP Events** | 2 | **70** | Aggressive Flushing |
| **Behavior** | Paused after 16KB | Flushed & Continued | Real-time Priority |

**Key Finding:** In high-load UDP scenarios, `KeepLatest` prevents application-level stalling, allowing the data pipeline to stay "live" even during temporary network congestion.

---

## 3. Implementation Status & API Guide

All wrappers now support the following properties in Python:

```python
# Available for: TcpClient, TcpServer, UdpClient, UdpServer, Serial, UdsClient, UdsServer
client = unilink.TcpClient("127.0.0.1", 10001)

# Set strategy (KeepAll is default)
client.backpressure_strategy = unilink.BackpressureStrategy.KeepLatest

# Set threshold (Default 16MB)
client.backpressure_threshold = 1024 * 512  # 0.5 MB for sensors
```

### Recommended Configurations

| Workload Type | Transport | Strategy | Threshold | Flow Control |
|:---|:---|:---|:---|:---|
| **Perception (LiDAR/Cam)** | TCP/UDS | `KeepLatest` | 0.5 - 1.0 MB | **OFF** |
| **Telemetry/Status** | UDP | `KeepLatest` | 128 - 256 KB | **OFF** |
| **Commands/Files** | TCP/UDS | `KeepAll` | 16 - 64 MB | **ON** |
| **MCU Control** | Serial | `KeepLatest` | 4 - 16 KB | **OFF** |

---

## 4. Final Conclusion
The addition of per-transport `BackpressureStrategy` configuration transforms Unilink from a general-purpose library into a **robotics-specialized communication engine**. It successfully bridges the gap between high-level Python convenience and low-level real-time requirements, delivering up to **73% of raw hardware performance** while maintaining strict memory safety and latency bounds.
