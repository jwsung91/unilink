"""
Unilink KeepLatest Strategy Latency Benchmark
==============================================
Compares KeepAll (standard) vs KeepLatest (real-time/sensor) backpressure strategies
under heavy load to demonstrate the latency improvement of the KeepLatest strategy.

Usage:
    python run_adaptive_benchmark.py
"""

import time
import sys
import os

sys.path.append(os.path.join(os.getcwd(), "build/bindings/python"))
import unilink_py as unilink


def run_bench(label, threshold_mb, strategy):
    print(f"\n>>> [{label}] threshold={threshold_mb}MB strategy={strategy}")

    server = unilink.TcpServer(10021)
    server.start()

    client = unilink.TcpClient("127.0.0.1", 10021)
    client.backpressure_threshold = int(threshold_mb * 1024 * 1024)
    client.backpressure_strategy = strategy

    received_count = 0
    latencies = []

    def on_msg(ctx):
        nonlocal received_count
        received_count += 1
        try:
            parts = ctx.data.decode().split("|", 1)
            sent_at = float(parts[0])
            latencies.append(time.time() - sent_at)
        except Exception:
            pass

    server.on_message(on_msg)
    client.start()
    time.sleep(0.5)

    duration = 3.0
    start = time.time()
    sent = 0
    # ~100 KB payload → roughly 20 MB/s to overwhelm a 0.5 MB threshold quickly
    payload = "X" * (100 * 1024)

    while time.time() - start < duration:
        msg = f"{time.time()}|{payload}"
        client.send(msg)
        sent += 1
        time.sleep(0.005)  # 5ms interval

    time.sleep(1.0)  # drain

    client.stop()
    server.stop()

    if latencies:
        avg = sum(latencies) / len(latencies) * 1000
        p99 = sorted(latencies)[int(len(latencies) * 0.99)] * 1000
        mx = max(latencies) * 1000
        drop_rate = 100.0 * (1 - received_count / sent) if sent > 0 else 0.0
        print(f"  Sent:        {sent}")
        print(f"  Received:    {received_count}  (drop rate: {drop_rate:.1f}%)")
        print(f"  Avg latency: {avg:.1f} ms")
        print(f"  P99 latency: {p99:.1f} ms")
        print(f"  Max latency: {mx:.1f} ms")
        return avg, p99
    else:
        print("  No data received — check connection.")
        return None, None


if __name__ == "__main__":
    print("Unilink Backpressure Strategy Benchmark")
    print("========================================")
    print("KeepAll  → queue everything until hard limit; high latency under load")
    print("KeepLatest → drop oldest data at threshold; low latency, some loss")

    avg_all, p99_all = run_bench("KeepAll  (16 MB)", 16, unilink.BackpressureStrategy.KeepAll)
    avg_lat, p99_lat = run_bench("KeepLatest (0.5 MB)", 0.5, unilink.BackpressureStrategy.KeepLatest)

    if avg_all and avg_lat:
        print("\n=== Summary ===")
        print(f"Avg latency  — KeepAll: {avg_all:.1f} ms   KeepLatest: {avg_lat:.1f} ms")
        print(f"P99 latency  — KeepAll: {p99_all:.1f} ms   KeepLatest: {p99_lat:.1f} ms")
        improvement = (avg_all - avg_lat) / avg_all * 100 if avg_all > 0 else 0
        print(f"Avg latency improvement: {improvement:.1f}%")
