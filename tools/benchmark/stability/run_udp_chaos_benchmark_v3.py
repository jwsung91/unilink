"""
Unilink UDP BackpressureStrategy Chaos Benchmark (V3)
=========================================================
Final robust version: No transport restarts, toggles "Mute" instead.
"""

import time
import threading
import statistics
import sys
import os

sys.path.append(os.path.join(os.getcwd(), "build/lib"))
sys.path.append(os.path.join(os.getcwd(), "build/bindings/python"))
import unilink_py as unilink

# Global spec
PAYLOAD_SIZE    = 1024       # 1 KB
SEND_SLEEP      = 0.0001     # 100 μs (Fast enough to fill queue)
DURATION        = 10         # 10 seconds
PORT_S          = 10080
PORT_C          = 10081
CHAOS_INTERVAL  = 3.0        # 3 seconds active
DOWN_TIME       = 2.0        # 2 seconds "muted"

def run_bench(label: str, strategy, threshold_kb: float, use_flow_control: bool) -> dict:
    print(f"\n>>> [{label}] threshold={threshold_kb} KB, Flow Control={use_flow_control}")

    sent_bytes      = 0
    recv_bytes      = 0
    bp_events       = 0
    snapshots       = []
    running         = True
    send_allowed    = True
    network_muted   = False
    current_queued  = 0
    
    # ── Configuration ─────────────────────────────────────────────────────────
    cfg_s = unilink.UdpConfig()
    cfg_s.local_port = PORT_S
    cfg_s.remote_address = "127.0.0.1"
    cfg_s.remote_port = PORT_C
    
    # ── Peer S (Receiver) ─────────────────────────────────────────────────────
    peer_s = unilink.UdpClient(cfg_s)
    def _count(ctx):
        nonlocal recv_bytes
        if not network_muted:
            recv_bytes += len(ctx.data)
    peer_s.on_data(_count)
    peer_s.start_sync()

    # ── Peer C (Sender) ───────────────────────────────────────────────────────
    # Use direct properties added to UdpClient
    peer_c = unilink.UdpClient(unilink.UdpConfig())
    peer_c.backpressure_threshold = int(threshold_kb * 1024)
    peer_c.backpressure_strategy = strategy
    
    # Manual remote config for sender
    peer_c_cfg = unilink.UdpConfig()
    peer_c_cfg.local_port = PORT_C
    peer_c_cfg.remote_address = "127.0.0.1"
    peer_c_cfg.remote_port = PORT_S
    # Re-init with full config to be safe
    peer_c = unilink.UdpClient(peer_c_cfg)
    peer_c.backpressure_threshold = int(threshold_kb * 1024)
    peer_c.backpressure_strategy = strategy

    def on_bp(queued: int) -> None:
        nonlocal bp_events, send_allowed, current_queued
        current_queued = queued
        limit = int(threshold_kb * 1024)
        if queued > limit * 0.5:
            bp_events += 1
            if use_flow_control: send_allowed = False
        else:
            if use_flow_control: send_allowed = True
            
    peer_c.on_backpressure(on_bp)
    peer_c.start_sync()

    # ── Chaos & Monitor ───────────────────────────────────────────────────────
    def chaos_monkey():
        nonlocal network_muted
        while running:
            time.sleep(CHAOS_INTERVAL)
            if not running: break
            print(f"    [Chaos] Muting Network (Simulate Drop)...", flush=True)
            network_muted = True
            time.sleep(DOWN_TIME)
            if not running: break
            print(f"    [Chaos] Unmuting Network...", flush=True)
            network_muted = False

    def monitor():
        prev = 0
        t_start = time.time()
        while running:
            time.sleep(1)
            now = sent_bytes
            rate = (now - prev) / (1024 * 1024)
            snapshots.append(rate)
            status = "OK" if send_allowed else "BLOCKED"
            print(f"    T+{int(time.time()-t_start)}s | {rate:>5.2f} MB/s | Queued: {current_queued/1024:>6.1f} KB | {status}", flush=True)
            prev = now

    # ── Sender ────────────────────────────────────────────────────────────────
    payload = b"A" * PAYLOAD_SIZE
    def sender():
        nonlocal sent_bytes, running
        t_end = time.time() + DURATION
        while time.time() < t_end:
            if send_allowed:
                if peer_c.send(payload):
                    sent_bytes += PAYLOAD_SIZE
            if SEND_SLEEP > 0:
                time.sleep(SEND_SLEEP)
        running = False

    threads = [
        threading.Thread(target=monitor, daemon=True),
        threading.Thread(target=chaos_monkey, daemon=True),
        threading.Thread(target=sender)
    ]
    for t in threads: t.start()
    threads[2].join(timeout=DURATION + 2) # Wait for sender
    running = False
    
    time.sleep(0.5)
    peer_c.stop(); peer_s.stop()

    sent_mb = sent_bytes / (1024 * 1024)
    recv_mb = recv_bytes / (1024 * 1024)
    delivery = recv_mb / sent_mb * 100 if sent_mb > 0 else 0
    avg_tp   = statistics.mean(snapshots) if snapshots else 0

    print(f"  >> Finished: Sent={sent_mb:.2f}MB, Recv={recv_mb:.2f}MB, Delivery={delivery:.2f}%, BP Events={bp_events}")
    return dict(sent_mb=sent_mb, recv_mb=recv_mb, delivery=delivery, avg_tp=avg_tp, bp_events=bp_events)

if __name__ == "__main__":
    print("=" * 70)
    print("Unilink UDP Robust Chaos Benchmark (V3)")
    print("=" * 70)
    # Use smaller thresholds to trigger BP easier on local loopback
    r_all = run_bench("KeepAll (128KB) + Flow Control", unilink.BackpressureStrategy.KeepAll, 128, True)
    time.sleep(1)
    r_lat = run_bench("KeepLatest (128KB) NO Flow Control", unilink.BackpressureStrategy.KeepLatest, 128, False)

    print("\n" + "=" * 70)
    print(f"{'Metric':30s} {'KeepAll+FC':>15s} {'KeepLatest+NoFC':>18s}")
    print("-" * 70)
    def row(name, a, b, fmt=".2f"):
        print(f"{name:30s} {a:>15{fmt}} {b:>18{fmt}}")
    row("Sent (MB)",            r_all["sent_mb"],   r_lat["sent_mb"])
    row("Received (MB)",        r_all["recv_mb"],   r_lat["recv_mb"])
    row("Delivery rate (%)",    r_all["delivery"],  r_lat["delivery"])
    row("Throughput avg (MB/s)",r_all["avg_tp"],    r_lat["avg_tp"])
    row("BP events",            float(r_all["bp_events"]), float(r_lat["bp_events"]), ".0f")
