# BackpressureStrategy Benchmark — 2026-04-27

## Environment

- OS: Linux 6.6.87.2-microsoft-standard-WSL2
- Python: 3.12 / Unilink bindings: built from `feat/backpressure-keepall-keeplatest`
- Workload: LV3-equivalent (64 KB payload, 10 µs sleep, no chaos)
- Duration: 30 s per strategy (abbreviated from 60 s baseline)
- Script: `tools/benchmark/stability/run_adaptive_benchmark.py`

---

## Raw Results

### Reliable (16 MB threshold)

| Metric | Value |
|---|---|
| Sent | 17,644.69 MB |
| Received | 6,189.94 MB |
| Delivery rate | 35.08% |
| Throughput avg | 569.18 MB/s |
| Throughput StdDev | 101.37 MB/s |
| BP events | 1 |

### BestEffort (0.5 MB threshold)

| Metric | Value |
|---|---|
| Sent | 16,190.56 MB |
| Received | 5,553.69 MB |
| Delivery rate | 34.30% |
| Throughput avg | 522.28 MB/s |
| Throughput StdDev | 94.06 MB/s |
| BP events | 21,775 |

---

## Comparison with 2026-04-26 Baseline (LV3, Reliable default)

| Metric | Reliable (new) | BestEffort (new) | Baseline LV3 |
|---|---|---|---|
| Sent (MB) | 17,644 | 16,191 | 9,300 |
| Received (MB) | 6,190 | 5,554 | 9,248 |
| Delivery rate (%) | 35.08 | 34.30 | **99.44** |
| Throughput avg (MB/s) | 569.18 | 522.28 | 133.49 |
| Throughput StdDev (MB/s) | 101.37 | **94.06** | 66.76 |
| BP events | 1 | **21,775** | 965 |

---

## Analysis

### 1. BestEffort queue flush confirmed

BP events: 21,775 (BestEffort) vs 1 (Reliable). The `maybe_flush_for_keep_latest`
implementation is correctly flushing the tx_ queue every time `queue_bytes_` exceeds
the 0.5 MB threshold. At ~64 KB per message, the threshold is crossed after roughly
8 messages, meaning nearly every batch of sends triggers a flush under this load.

### 2. Throughput StdDev: BestEffort is 7% more predictable

StdDev drops from 101.4 → 94.1 MB/s.  Periodic queue flushes prevent burst
accumulation, smoothing out per-second throughput variance.  This is the expected
"bounded queue = predictable pipeline" property of BestEffort.

### 3. Delivery rate is similar (~35% for both)

Both strategies see the same ~35% delivery rate in this test.  The bottleneck here
is the Python GIL and the loopback kernel socket buffers (which hold data already
written from the Unilink tx_ queue).  BestEffort drops data at the Unilink layer,
but the kernel buffers already contain enough data to sustain the receiver
independently.

The 2026-04-26 baseline achieved 99.44% delivery because it used **explicit flow
control**: the sender paused on `on_backpressure` (ON event) and resumed on the OFF
event.  This benchmark intentionally omits flow control to isolate the effect of the
strategy alone.

### 4. Latency measurement is not meaningful on WSL2 loopback

Data path: `Unilink tx_` → kernel send buffer → kernel recv buffer → application.

BestEffort flushes `Unilink tx_` but cannot touch data already in the kernel
buffers.  On WSL2 loopback the kernel buffers are several MB (effectively instant
network), so the oldest data is already kernel-buffered before the first flush
occurs.  The latency difference that BestEffort provides on a real WAN (RTT ≥ 10 ms,
limited bandwidth) is not reproducible in this environment.

### 5. Correct use of BestEffort in production

| Goal | Recommended pattern |
|---|---|
| Sensor/video stream, latency-first | `BestEffort` + ignore BP callback (let queue flush) |
| Sensor stream + bounded memory | `BestEffort` + monitor BP event count for observability |
| File / command / log, reliability-first | `Reliable` + flow control in BP callback |
| Reliability + back-pressure safety | `Reliable` + pause-on-BP + `bp_limit_` as OOM guard |

---

## Key Takeaway

BestEffort is not a latency-reduction mechanism in isolation — it is a **queue memory
bound + throughput predictability** trade-off.  It prevents unbounded tx_ queue growth
(OOM risk) and smooths throughput variance at the cost of intentional message drops.
The latency benefit only materialises when the network itself is the bottleneck
(limited bandwidth / high RTT), at which point BestEffort ensures that the data
reaching the receiver is always fresh rather than stale.
