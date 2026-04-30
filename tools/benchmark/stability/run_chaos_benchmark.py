"""
Unilink BackpressureStrategy Real-World Chaos Benchmark
=========================================================
Compares Reliable with Flow Control vs BestEffort without Flow Control 
under LV3-equivalent chaos conditions (64 KB, 10 μs sleep, 7s chaos interval).
"""

import time
import threading
import statistics
import sys
import os

sys.path.append(os.path.join(os.getcwd(), "build/lib"))
sys.path.append(os.path.join(os.getcwd(), "build/bindings/python"))
import unilink_py as unilink

# LV3 spec
PAYLOAD_SIZE    = 65_536   # 64 KB
SEND_SLEEP      = 0.00001  # 10 μs
DURATION        = 60       # seconds
PORT_BASE       = 10050
CHAOS_INTERVAL  = 7.0      # seconds
DOWN_TIME       = 2.0      # seconds

def run_bench(label: str, strategy, threshold_mb: float, port: int, use_flow_control: bool) -> dict:
    print(f"\n>>> [{label}] threshold={threshold_mb} MB, Flow Control={use_flow_control}")

    sent_bytes      = 0
    recv_bytes      = 0
    bp_events       = 0
    snapshots       = []
    running         = True
    send_allowed    = True

    # ── server / chaos monkey ────────────────────────────────────────────────
    server = unilink.TcpServer(port)
    
    def _count(ctx):
        nonlocal recv_bytes
        recv_bytes += len(ctx.data)
        
    server.on_data(_count)
    server.start_sync()

    def chaos_monkey():
        while running:
            time.sleep(CHAOS_INTERVAL)
            if not running: break
            # Drop connection
            server.stop()
            time.sleep(DOWN_TIME)
            if not running: break
            # Restart server
            server.start_sync()

    # ── client ────────────────────────────────────────────────────────────────
    client = unilink.TcpClient("127.0.0.1", port)
    client.backpressure_threshold = int(threshold_mb * 1024 * 1024)
    client.backpressure_strategy  = strategy

    def on_bp(queued: int) -> None:
        nonlocal bp_events, send_allowed
        # threshold_mb * 1024 * 1024
        limit = int(threshold_mb * 1024 * 1024)
        
        # BestEffort flushes to 0. Reliable might hover around limit.
        # High watermark
        if queued > limit // 2:
            bp_events += 1
            if use_flow_control:
                send_allowed = False
        else:
            if use_flow_control:
                send_allowed = True

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
                    # If flow control is enabled and we hit BP, we pause sending
                    if send_allowed:
                        if client.send(payload):
                            sent_bytes += PAYLOAD_SIZE
                    
                    # We sleep either way to prevent GIL starvation
                    if SEND_SLEEP > 0:
                        time.sleep(SEND_SLEEP)
            except Exception:
                pass
        running = False

    mon_t = threading.Thread(target=monitor, daemon=True)
    chaos_t = threading.Thread(target=chaos_monkey, daemon=True)
    snd_t = threading.Thread(target=sender)
    
    mon_t.start()
    chaos_t.start()
    snd_t.start()
    
    snd_t.join()
    time.sleep(1)

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
    print("Unilink Real-World Chaos Benchmark (LV3 spec)")
    print(f"Payload: 64 KB | Sleep: 10 µs | Duration: {DURATION}s")
    print(f"Chaos: {CHAOS_INTERVAL}s interval, {DOWN_TIME}s down time")
    print("=" * W)

    r_all = run_bench("Reliable (512 KB) + Flow Control", unilink.BackpressureStrategy.Reliable, 0.5, PORT_BASE, True)
    r_lat = run_bench("BestEffort (0.5 MB) NO Flow Control", unilink.BackpressureStrategy.BestEffort, 0.5, PORT_BASE + 1, False)

    print("\n" + "=" * W)
    print("Summary")
    print("=" * W)

    hdr = f"{'Metric':30s} {'Reliable+FC':>14s} {'BestEffort+NoFC':>16s}"
    print(hdr)
    print("-" * W)

    def row(name, a, b, fmt=".2f"):
        print(f"{name:30s} {a:>14{fmt}} {b:>16{fmt}}")

    row("Sent (MB)",            r_all["sent_mb"],   r_lat["sent_mb"])
    row("Received (MB)",        r_all["recv_mb"],   r_lat["recv_mb"])
    row("Delivery rate (%)",    r_all["delivery"],  r_lat["delivery"])
    row("Throughput avg (MB/s)",r_all["avg_tp"],    r_lat["avg_tp"])
    row("Throughput StdDev",    r_all["std_tp"],    r_lat["std_tp"])
    row("BP events",            float(r_all["bp_events"]), float(r_lat["bp_events"]), ".0f")
