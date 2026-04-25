# TCP Benchmark: Python socket vs Unilink

This benchmark compares the performance of the standard Python `socket` module and `unilink.TcpClient`/`TcpServer`.

## Results Summary

| Test Case | Python (sec) | Unilink (sec) |
|-----------|--------------|---------------|
| 100 Pings | ~0.0030 | ~0.0090 |
| 500 Pings | ~0.0150 | ~0.0500 |
| 1000 Pings | ~0.0300 | ~0.0950 |
| 1000 Chunks (1MB total) | ~0.0020 | ~0.0070 |

## Analysis
- **Latency**: Python's native `socket` is faster for small ping-pong messages due to minimal abstraction. Unilink's overhead comes from its asynchronous event loop and the C++/Python bridge (GIL acquisition).
- **Stability**: Very stable across all tested loads.
