# UDS Benchmark: Python socket vs Unilink

This benchmark compares the performance of the standard Python `socket` module (AF_UNIX) and `unilink.UdsClient`/`UdsServer`.

## Results Summary

| Test Case | Python (sec) | Unilink (sec) |
|-----------|--------------|---------------|
| 100 Pings | ~0.0030 | ~0.0100 |
| 500 Pings | ~0.0150 | ~0.0500 |
| 1000 Pings | ~0.0250 | ~0.0900 |
| 100 Chunks (100KB total) | ~0.0005 | ~0.0020 |

## Analysis
- **Stability**: During high-load throughput tests (e.g., streaming 500+ KB rapidly), a segmentation fault can occur. This suggests a race condition in the Python binding or the C++ UDS transport layer during intense I/O or rapid shutdown.
- **Latency**: UDS shows low latency as expected for IPC, with Unilink's abstraction overhead remaining consistent with other transports.
