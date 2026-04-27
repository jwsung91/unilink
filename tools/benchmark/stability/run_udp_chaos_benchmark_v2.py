"""
Unilink UDP BackpressureStrategy Chaos Benchmark (V2)
=========================================================
Robust version with better visibility and fixed UDP params.
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
PAYLOAD_SIZE    = 1024       # 1 KB (Safe for UDP)
SEND_SLEEP      = 0.0005     # 500 μs
DURATION        = 15         # 15 seconds
PORT_S          = 10070
PORT_C          = 10071
CHAOS_INTERVAL  = 4.0        # 4 seconds active
DOWN_TIME       = 2.0        # 2 seconds down

def run_bench(label: str, strategy, threshold_kb: float, use_flow_control: bool) -> dict:
    print(f"\n>>> [{label}] threshold={threshold_kb} KB, Flow Control={use_flow_control}")

    sent_bytes      = 0
    recv_bytes      = 0
    bp_events       = 0
    snapshots       = []
    running         = True
    send_allowed    = True
    
    # ── Configuration ─────────────────────────────────────────────────────────
    cfg_s = unilink.UdpConfig()
    cfg_s.local_port = PORT_S
    cfg_s.remote_address = "127.0.0.1"
    cfg_s.remote_port = PORT_C
    
    cfg_c = unilink.UdpConfig()
    cfg_c.local_port = PORT_C
    cfg_c.remote_address = "127.0.0.1"
    cfg_c.remote_port = PORT_S
    cfg_c.backpressure_threshold = int(threshold_kb * 1024)
    cfg_c.backpressure_strategy = strategy

    # ── Peer S (Receiver) ─────────────────────────────────────────────────────
    peer_s = unilink.UdpClient(cfg_s)
    def _count(ctx):
        nonlocal recv_bytes
        recv_bytes += len(ctx.data)
    peer_s.on_data(_count)
    peer_s.start_sync()

    # ── Peer C (Sender) ───────────────────────────────────────────────────────
    peer_c = unilink.UdpClient(cfg_c)
    def on_bp(queued: int) -> None:
        nonlocal bp_events, send_allowed
        limit = int(threshold_kb * 1024)
        if queued > limit * 0.8: # High watermark
            bp_events += 1
            if use_flow_control: send_allowed = False
        elif queued < limit * 0.2: # Low watermark
            if use_flow_control: send_allowed = True
            
    peer_c.on_backpressure(on_bp)
    peer_c.start_sync()

    # ── Chaos & Monitor ───────────────────────────────────────────────────────
    def chaos_monkey():
        time.sleep(2) # Warmup
        while running:
            time.sleep(CHAOS_INTERVAL)
            if not running: break
            print(f"  [Chaos] NETWORK DOWN (Stopping Receiver)", flush=True)
            peer_s.stop()
            time.sleep(DOWN_TIME)
            if not running: break
            print(f"  [Chaos] NETWORK UP (Restarting Receiver)", flush=True)
            peer_s.start_sync()

    def monitor():
        prev = 0
        t_start = time.time()
        while running and (time.time() - t_start < DURATION + 2):
            time.sleep(1)
            now = sent_bytes
            rate = (now - prev) / (1024 * 1024)
            snapshots.append(rate)
            status = "SENDING" if send_allowed else "PAUSED (BP)"
            print(f"    T+{int(time.time()-t_start)}s | Sent: {rate:>5.2f} MB/s | Recv Total: {recv_bytes/(1024*1024):>6.2f} MB | {status}", flush=True)
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

    mon_t = threading.Thread(target=monitor, daemon=True)
    chaos_t = threading.Thread(target=chaos_monkey, daemon=True)
    snd_t = threading.Thread(target=sender)
    
    mon_t.start(); chaos_t.start(); snd_t.start()
    snd_t.join(timeout=DURATION + 5)
    running = False
    
    time.sleep(1)
    peer_c.stop(); peer_s.stop()

    sent_mb = sent_bytes / (1024 * 1024)
    recv_mb = recv_bytes / (1024 * 1024)
    delivery = recv_mb / sent_mb * 100 if sent_mb > 0 else 0
    avg_tp   = statistics.mean(snapshots) if snapshots else 0

    print(f"  >> Finished: Sent={sent_mb:.2f}MB, Recv={recv_mb:.2f}MB, Delivery={delivery:.2f}%, BP={bp_events}")
    return dict(sent_mb=sent_mb, recv_mb=recv_mb, delivery=delivery, avg_tp=avg_tp, bp_events=bp_events)

if __name__ == "__main__":
    print("=" * 70)
    print("Unilink UDP Robust Chaos Benchmark")
    print("=" * 70)
    r_all = run_bench("KeepAll (2MB) + Flow Control", unilink.BackpressureStrategy.KeepAll, 2048, True)
    time.sleep(2)
    r_lat = run_bench("KeepLatest (256KB) NO Flow Control", unilink.BackpressureStrategy.KeepLatest, 256, False)

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
