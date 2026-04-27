"""
Unilink UDS BackpressureStrategy Chaos Benchmark
=========================================================
Compares KeepAll with Flow Control vs KeepLatest without Flow Control 
under UDS (Unix Domain Sockets) local IPC chaos conditions.
"""

import time
import threading
import statistics
import sys
import os

sys.path.append(os.path.join(os.getcwd(), "build/lib"))
sys.path.append(os.path.join(os.getcwd(), "build/bindings/python"))
import unilink_py as unilink

# Spec for high-speed local IPC
PAYLOAD_SIZE    = 1024 * 128  # 128 KB (Larger for UDS)
SEND_SLEEP      = 0.000001    # 1 μs (Extremely fast)
DURATION        = 15          # 15 seconds
SOCK_PATH       = "/tmp/unilink_bench.sock"
CHAOS_INTERVAL  = 3.0         # 3s active
DOWN_TIME       = 1.0         # 1s down

def run_bench(label: str, strategy, threshold_mb: float, use_flow_control: bool) -> dict:
    print(f"\n>>> [{label}] threshold={threshold_mb} MB, Flow Control={use_flow_control}")

    if os.path.exists(SOCK_PATH): os.remove(SOCK_PATH)

    sent_bytes      = 0
    recv_bytes      = 0
    bp_events       = 0
    snapshots       = []
    running         = True
    send_allowed    = True

    # ── Server ────────────────────────────────────────────────────────────────
    server = unilink.UdsServer(SOCK_PATH)
    def _count(ctx):
        nonlocal recv_bytes
        recv_bytes += len(ctx.data)
    server.on_data(_count)
    server.start_sync()

    # ── Client ────────────────────────────────────────────────────────────────
    client = unilink.UdsClient(SOCK_PATH)
    client.backpressure_threshold = int(threshold_mb * 1024 * 1024)
    client.backpressure_strategy  = strategy

    def on_bp(queued: int) -> None:
        nonlocal bp_events, send_allowed
        limit = int(threshold_mb * 1024 * 1024)
        if queued > limit * 0.8:
            bp_events += 1
            if use_flow_control: send_allowed = False
        elif queued < limit * 0.2:
            if use_flow_control: send_allowed = True

    client.on_backpressure(on_bp)
    client.start_sync()

    # ── Chaos Monkey ──────────────────────────────────────────────────────────
    def chaos_monkey():
        while running:
            time.sleep(CHAOS_INTERVAL)
            if not running: break
            # stop reader indirectly by stopping server
            server.stop()
            time.sleep(DOWN_TIME)
            if not running: break
            server.start_sync()

    # ── Monitor ───────────────────────────────────────────────────────────────
    def monitor():
        prev = 0
        t_start = time.time()
        while running:
            time.sleep(1)
            now = sent_bytes
            rate = (now - prev) / (1024 * 1024)
            snapshots.append(rate)
            status = "OK" if send_allowed else "BLOCKED"
            print(f"    T+{int(time.time()-t_start)}s | {rate:>7.2f} MB/s | Recv: {recv_bytes/(1024*1024):>7.2f} MB | {status}", flush=True)
            prev = now

    # ── Sender ────────────────────────────────────────────────────────────────
    payload = b"A" * PAYLOAD_SIZE
    def sender():
        nonlocal sent_bytes, running
        t_end = time.time() + DURATION
        while time.time() < t_end:
            if send_allowed:
                if client.send(payload):
                    sent_bytes += PAYLOAD_SIZE
            if SEND_SLEEP > 0:
                time.sleep(SEND_SLEEP)
        running = False

    mon_t = threading.Thread(target=monitor, daemon=True)
    chaos_t = threading.Thread(target=chaos_monkey, daemon=True)
    snd_t = threading.Thread(target=sender)
    
    mon_t.start(); chaos_t.start(); snd_t.start()
    snd_t.join(timeout=DURATION + 5)
    running = False
    
    time.sleep(0.5)
    client.stop(); server.stop()
    if os.path.exists(SOCK_PATH): os.remove(SOCK_PATH)

    sent_mb = sent_bytes / (1024 * 1024)
    recv_mb = recv_bytes / (1024 * 1024)
    delivery = recv_mb / sent_mb * 100 if sent_mb > 0 else 0
    avg_tp   = statistics.mean(snapshots) if snapshots else 0

    print(f"  >> Finished: Sent={sent_mb:.2f}MB, Recv={recv_mb:.2f}MB, Delivery={delivery:.2f}%, BP={bp_events}")
    return dict(sent_mb=sent_mb, recv_mb=recv_mb, delivery=delivery, avg_tp=avg_tp, bp_events=bp_events)

if __name__ == "__main__":
    print("=" * 75)
    print("Unilink UDS Local IPC Chaos Benchmark")
    print("=" * 75)

    r_all = run_bench("KeepAll (16MB) + Flow Control", unilink.BackpressureStrategy.KeepAll, 16, True)
    time.sleep(1)
    r_lat = run_bench("KeepLatest (1MB) NO Flow Control", unilink.BackpressureStrategy.KeepLatest, 1, False)

    print("\n" + "=" * 75)
    print(f"{'Metric':30s} {'KeepAll+FC':>18s} {'KeepLatest+NoFC':>20s}")
    print("-" * 75)
    def row(name, a, b, fmt=".2f"):
        print(f"{name:30s} {a:>18{fmt}} {b:>20{fmt}}")

    row("Sent (MB)",            r_all["sent_mb"],   r_lat["sent_mb"])
    row("Received (MB)",        r_all["recv_mb"],   r_lat["recv_mb"])
    row("Delivery rate (%)",    r_all["delivery"],  r_lat["delivery"])
    row("Throughput avg (MB/s)",r_all["avg_tp"],    r_lat["avg_tp"])
    row("BP events",            float(r_all["bp_events"]), float(r_lat["bp_events"]), ".0f")
