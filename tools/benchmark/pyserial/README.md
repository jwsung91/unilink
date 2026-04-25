# Python Serial Benchmark: pyserial vs unilink

This directory contains a benchmark script (`benchmark.py`) designed to compare the performance of `pyserial` and `unilink.Serial` when communicating over virtual serial ports.

## Setup & Requirements

1. **Virtual Serial Ports**: The benchmark uses `socat` to create a pair of connected virtual PTYs (pseudo-terminals).
   - Ensure `socat` is installed on your system: `sudo apt-get install socat`
2. **Python Dependencies**:
   - `pyserial` (`pip install pyserial`)
   - `unilink` (ensure the Python bindings are built and accessible in your `PYTHONPATH`)

## Running the Benchmark

You can run the benchmark using:

```bash
python3 benchmark.py
```

*Note: If testing against a local build of Unilink, make sure to set the `PYTHONPATH` accordingly.*

```bash
PYTHONPATH=$(pwd)/../../../bindings/python:$(pwd)/../../../build/lib:$PYTHONPATH python3 benchmark.py
```

## Benchmark Details

The test suites evaluate two common communication patterns:

1. **Progressive Latency Test (Ping-Pong)**: 
   - Sends a small 4-byte message ("PING") back and forth.
   - Measures the total time to complete a given number of ping-pong cycles.
   - Evaluates the overhead of context switching and event loop dispatching for very short, frequent messages.

2. **Progressive Throughput Test (Streaming)**:
   - Streams 1KB chunks of data continuously from a master to a slave.
   - Measures the total time to transmit the full payload.
   - Evaluates bulk data transfer efficiency.

## Benchmark Results

*Run on local environment, April 2026*

### 1. Progressive Latency Test (Ping-Pong, 4 bytes/msg)

| Messages | `pyserial` (sec) | `unilink` (sec) |
|----------|------------------|-----------------|
| 10       | 0.0019           | 0.0022          |
| 50       | 0.0075           | 0.0109          |
| 100      | 0.0116           | 0.0175          |
| 500      | 0.0585           | 0.0853          |
| 1000     | 0.1087           | 0.1674          |

*Analysis*: For extremely small, high-frequency "ping-pong" style communication, `pyserial` demonstrates slightly lower latency. This is expected, as `unilink` operates an internal asynchronous event loop and invokes C++ background threads for I/O operations. For a 4-byte payload, the context switching between C++ and Python (including GIL acquisition) creates a marginal, but measurable, overhead.

### 2. Progressive Throughput Test (Streaming, 1KB/chunk)

| Chunks (1KB) | `pyserial` (sec) | `unilink` (sec) |
|--------------|------------------|-----------------|
| 10           | 0.0108           | 0.0109          |
| 50           | 0.0541           | 0.0546          |
| 100          | 0.1084           | 0.1094          |
| 500          | 0.5439           | 0.5438          |

*Analysis*: In a continuous streaming scenario with larger chunks, the performance between `pyserial` and `unilink` is nearly identical. In this regime, the bottleneck is no longer the library's internal overhead, but rather the OS-level PTY I/O throughput.

## Known Issues

When running large iteration tests using `socat` with the `-d -d` flags (for parsing PTY names from `stderr`), OS pipe buffers (typically 64KB) can fill up with debug logs, causing `socat` to block and resulting in a deadlock.

**Fix implemented in `benchmark.py`**:
- A background daemon thread continuously reads and drains `socat`'s `stderr` after the initial setup.
- Write timeouts (`write_timeout=2`) are added to the `pyserial` constructors as a safety measure to fail fast rather than hanging indefinitely if the PTY blocks.
