"""
Unilink UDP BackpressureStrategy Chaos Benchmark
=========================================================
Compares Reliable with Flow Control vs BestEffort without Flow Control 
under UDP load conditions.
"""

import time
import threading
import statistics
import sys
import os

sys.path.append(os.path.join(os.getcwd(), "build/lib"))
sys.path.append(os.path.join(os.getcwd(), "build/bindings/python"))
import unilink_py as unilink

# Benchmark spec
PAYLOAD_SIZE    = 1024       # 1 KB (Avoid truncation errors)
SEND_SLEEP      = 0.001      # 1 ms (Python friendly)
DURATION        = 20         # 20 seconds
PORT_S          = 10060
PORT_C          = 10061
CHAOS_INTERVAL  = 5.0        # 5 seconds
DOWN_TIME       = 1.0        # 1 second

def run_bench(label: str, strategy, threshold_mb: float, use_flow_control: bool) -> dict:
    print(f"\n>>> [{label}] threshold={threshold_mb} MB, Flow Control={use_flow_control}")

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
    # We can now set these on UdpConfig OR the instance directly
    cfg_c.backpressure_threshold = int(threshold_mb * 1024 * 1024)
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
        limit = int(threshold_mb * 1024 * 1024)
        if queued > limit // 2:
            bp_events += 1
            if use_flow_control:
                send_allowed = False
        else:
            if use_flow_control:
                send_allowed = True

    peer_c.on_backpressure(on_bp)
    peer_c.start_sync()

    # ── Chaos Monkey ──────────────────────────────────────────────────────────
    def chaos_monkey():
        while running:
            time.sleep(CHAOS_INTERVAL)
            if not running: break
            # print(f"  [{label}] Chaos: Stopping receiver...")
            peer_s.stop()
            time.sleep(DOWN_TIME)
            if not running: break
            # print(f"  [{label}] Chaos: Restarting receiver...")
            peer_s.start_sync()

    # ── Monitor ───────────────────────────────────────────────────────────────
    def monitor():
        prev = 0
        while running:
            time.sleep(1)
            now = sent_bytes
            rate = (now - prev) / (1024 * 1024)
            snapshots.append(rate)
            print(f"  Progress: {rate:>6.2f} MB/s | Recv: {recv_bytes/(1024*1024):>6.2f} MB", flush=True)
            prev = now

    # ── Sender ────────────────────────────────────────────────────────────────
    payload = b"A" * PAYLOAD_SIZE

    def sender():
        nonlocal sent_bytes, running
        t_end = time.time() + DURATION
        try:
            while time.time() < t_end:
                if send_allowed:
                    if peer_c.send(payload):
                        sent_bytes += PAYLOAD_SIZE
                
                if SEND_SLEEP > 0:
                    time.sleep(SEND_SLEEP)
        except Exception as e:
            print(f"Sender error: {e}")
        running = False

    mon_t = threading.Thread(target=monitor, daemon=True)
    chaos_t = threading.Thread(target=chaos_monkey, daemon=True)
    snd_t = threading.Thread(target=sender)
    
    mon_t.start()
    chaos_t.start()
    snd_t.start()
    
    # Force join with timeout to prevent hanging
    snd_t.join(timeout=DURATION + 5.0)
    if snd_t.is_alive():
        print(f"  !!! Warning: [{label}] Sender thread timed out and was forced to terminate.")
    
    running = False
    time.sleep(0.5)

    peer_c.stop()
    peer_s.stop()

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
    print("Unilink UDP Chaos Benchmark")
    print(f"Payload: {PAYLOAD_SIZE/1024:.1f} KB | Sleep: {SEND_SLEEP*1e6:.0f} µs | Duration: {DURATION}s")
    print(f"Chaos: {CHAOS_INTERVAL}s interval, {DOWN_TIME}s down time (Receiver Stop)")
    print("=" * W)

    r_all = run_bench("Reliable (8 MB) + Flow Control", unilink.BackpressureStrategy.Reliable, 8, True)
    r_lat = run_bench("BestEffort (0.5 MB) NO Flow Control", unilink.BackpressureStrategy.BestEffort, 0.5, False)

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
