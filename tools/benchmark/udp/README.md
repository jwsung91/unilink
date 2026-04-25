# UDP Benchmark: Python socket vs Unilink

This benchmark compares the performance of the standard Python `socket` module (DGRAM) and `unilink.UdpClient`.

## Results Summary

| Test Case | Python (sec) | Unilink (sec) |
|-----------|--------------|---------------|
| 100 Pings | ~0.0035 | ~0.0095 |
| 100 Chunks (100KB total) | ~0.0020 | ~0.0028 |

## Analysis
- **High Load**: At very high message rates (500+ pings/s), Unilink's UDP transport may experience event queue congestion or packet loss on local loopback, leading to timeouts in this benchmark's synchronized test pattern.
- **Throughput**: For moderate streaming, Unilink performs close to native sockets.
