"""
Unilink BackpressureStrategy Comparison Benchmark
==================================================
Compares KeepAll vs KeepLatest under LV3-equivalent sustained load
(64 KB payload, max rate, no chaos) and reports the same metrics used
in the 2026-04-26 stability baseline so results can be compared directly.

Why latency is NOT measured here
---------------------------------
On WSL2 loopback the TCP kernel receive buffer (~128 KB) fills before
Unilink's tx_ queue even starts, so end-to-end latency is dominated by
kernel-buffer draining — the same for both strategies.  The meaningful
difference is in queue-management behaviour: throughput variance,
delivery rate under overload, and memory pressure.

Usage:
    cd <project-root>
    python3 tools/benchmark/stability/run_adaptive_benchmark.py
"""

import time
import threading
import statistics
import sys
import os

sys.path.append(os.path.join(os.getcwd(), "build/lib"))
sys.path.append(os.path.join(os.getcwd(), "build/bindings/python"))
import unilink_py as unilink

# LV3 equivalent — same as 2026-04-26 baseline (Python column)
PAYLOAD_SIZE    = 65_536   # 64 KB
SEND_SLEEP      = 0.00001  # 10 μs  (GIL safety, same as LV3)
DURATION        = 30       # seconds (abbreviated from 60 s baseline)
PORT_BASE       = 10040    # separate from other benches


def run_bench(label: str, strategy, threshold_mb: float, port: int) -> dict:
    print(f"\n>>> [{label}]  threshold={threshold_mb} MB  duration={DURATION}s")

    sent_bytes      = 0
    recv_bytes      = 0
    bp_events       = 0
    snapshots       = []
    running         = True

    # ── server ────────────────────────────────────────────────────────────────
    server = unilink.TcpServer(port)
    server.on_data(lambda ctx: _count(ctx))

    def _count(ctx):
        nonlocal recv_bytes
        recv_bytes += len(ctx.data)

    server.on_data(_count)
    server.start_sync()

    # ── client ────────────────────────────────────────────────────────────────
    client = unilink.TcpClient("127.0.0.1", port)
    client.backpressure_threshold = int(threshold_mb * 1024 * 1024)
    client.backpressure_strategy  = strategy

    def on_bp(queued: int) -> None:
        nonlocal bp_events
        # ON event: queued >= bp_high_; OFF event: queued <= bp_low_ (== 0 for KeepLatest flush)
        if queued > (threshold_mb * 1024 * 1024) // 2:
            bp_events += 1

    client.on_backpressure(on_bp)
    client.start_sync()

    # ── monitor ───────────────────────────────────────────────────────────────
    def monitor():
        prev = 0
        while running:
            time.sleep(1)
            now = sent_bytes
            snapshots.append((now - prev) / (1024 * 1024))
            prev = now

    # ── sender ────────────────────────────────────────────────────────────────
    payload = b"A" * PAYLOAD_SIZE

    def sender():
        nonlocal sent_bytes, running
        t_end = time.time() + DURATION
        while time.time() < t_end:
            try:
                if client.connected():
                    if client.send(payload):
                        sent_bytes += PAYLOAD_SIZE
                    if SEND_SLEEP > 0:
                        time.sleep(SEND_SLEEP)
            except Exception:
                pass
        running = False

    mon_t = threading.Thread(target=monitor, daemon=True)
    snd_t = threading.Thread(target=sender)
    mon_t.start()
    snd_t.start()
    snd_t.join()
    time.sleep(1)  # final snapshot

    client.stop()
    server.stop()

    sent_mb = sent_bytes / (1024 * 1024)
    recv_mb = recv_bytes / (1024 * 1024)
    delivery = recv_mb / sent_mb * 100 if sent_mb > 0 else 0
    avg_tp   = statistics.mean(snapshots) if snapshots else 0
    std_tp   = statistics.stdev(snapshots) if len(snapshots) > 1 else 0

    print(f"  Sent:     {sent_mb:>10.2f} MB")
    print(f"  Received: {recv_mb:>10.2f} MB   Delivery: {delivery:.2f}%")
    print(f"  Throughput avg: {avg_tp:.2f} MB/s   StdDev: {std_tp:.2f} MB/s")
    print(f"  BP events: {bp_events}")

    return dict(sent_mb=sent_mb, recv_mb=recv_mb, delivery=delivery,
                avg_tp=avg_tp, std_tp=std_tp, bp_events=bp_events)


if __name__ == "__main__":
    W = 72
    print("=" * W)
    print("Unilink BackpressureStrategy Stability Comparison")
    print(f"Payload: {PAYLOAD_SIZE//1024} KB | Sleep: {SEND_SLEEP*1e6:.0f} µs | Duration: {DURATION}s (no chaos)")
    print("=" * W)

    r_all = run_bench("KeepAll   (16 MB)",  unilink.BackpressureStrategy.KeepAll,   16,  PORT_BASE)
    r_lat = run_bench("KeepLatest (0.5 MB)", unilink.BackpressureStrategy.KeepLatest, 0.5, PORT_BASE + 1)

    print("\n" + "=" * W)
    print("Results vs 2026-04-26 Baseline  (Python, LV3, 60 s, KeepAll default)")
    print("=" * W)

    hdr = f"{'Metric':30s} {'KeepAll (new)':>14s} {'KeepLatest (new)':>16s} {'Baseline LV3':>13s}"
    print(hdr)
    print("-" * W)

    def row(name, a, b, base, fmt=".2f"):
        print(f"{name:30s} {a:>14{fmt}} {b:>16{fmt}} {base:>13{fmt}}")

    row("Sent (MB)",            r_all["sent_mb"],   r_lat["sent_mb"],    9300.56)
    row("Received (MB)",        r_all["recv_mb"],   r_lat["recv_mb"],    9248.53)
    row("Delivery rate (%)",    r_all["delivery"],  r_lat["delivery"],   99.44)
    row("Throughput avg (MB/s)",r_all["avg_tp"],    r_lat["avg_tp"],     133.49)
    row("Throughput StdDev",    r_all["std_tp"],    r_lat["std_tp"],     66.76)
    row("BP events",            float(r_all["bp_events"]), float(r_lat["bp_events"]), 965.0, ".0f")

    print()
    print("Notes:")
    print("  Baseline: 60 s run with chaos/reconnect (LV3, Python Unilink, KeepAll default)")
    print("  New:      30 s run, no chaos, both strategies, same LV3 load params")
    print("  KeepLatest: higher BP events + lower delivery is expected (intentional drops)")
    print("  Lower StdDev for KeepLatest = more predictable throughput under queue pressure")
