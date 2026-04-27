"""
Unilink UDP Comprehensive Stability Benchmark
=========================================================
Compares:
1. Python Raw UDP Sockets
2. Unilink UDP KeepAll + Flow Control
3. Unilink UDP KeepLatest (No Flow Control)

Under LV3 Load: 64KB payloads, 10us sleep, 7s chaos.
"""

import socket
import time
import threading
import statistics
import sys
import os

sys.path.append(os.path.join(os.getcwd(), "build/lib"))
sys.path.append(os.path.join(os.getcwd(), "build/bindings/python"))
import unilink_py as unilink

# LV3 spec (UDP friendly)
PAYLOAD_SIZE    = 4000       # Under 4KB default buffer
SEND_SLEEP      = 0.00001    # 10us
DURATION        = 10
PORT            = 10090
CHAOS_INTERVAL  = 4.0
DOWN_TIME       = 1.0

# --- Raw UDP Benchmark ---
def run_raw_udp():
    print("\n>>> [Raw UDP Socket] Starting...")
    sent_bytes = 0
    recv_bytes = 0
    running = True
    muted = False
    
    def server():
        nonlocal recv_bytes
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024*1024*10) # 10MB buf
            s.bind(("127.0.0.1", PORT))
            s.settimeout(0.5)
            while running:
                try:
                    data, _ = s.recvfrom(65536)
                    if not muted:
                        recv_bytes += len(data)
                except socket.timeout:
                    continue
                except Exception as e:
                    print(f"Server error: {e}")
                    break

    def chaos():
        nonlocal muted
        while running:
            time.sleep(CHAOS_INTERVAL)
            muted = True
            time.sleep(DOWN_TIME)
            muted = False

    t_srv = threading.Thread(target=server, daemon=True)
    t_cho = threading.Thread(target=chaos, daemon=True)
    t_srv.start(); t_cho.start()

    payload = b"A" * PAYLOAD_SIZE
    start_time = time.time()
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as c:
        while time.time() - start_time < DURATION:
            try:
                c.sendto(payload, ("127.0.0.1", PORT))
                sent_bytes += PAYLOAD_SIZE
                if SEND_SLEEP > 0: time.sleep(SEND_SLEEP)
            except Exception as e:
                # print(f"Client error: {e}")
                pass
    
    running = False
    t_srv.join(timeout=1.0)
    
    sent_mb = sent_bytes / (1024 * 1024)
    recv_mb = recv_bytes / (1024 * 1024)
    delivery = (recv_mb / sent_mb * 100) if sent_mb > 0 else 0
    print(f"    Sent: {sent_mb:.2f} MB | Recv: {recv_mb:.2f} MB | Delivery: {delivery:.2f}%")
    return delivery, sent_mb / DURATION

# --- Unilink UDP Benchmark ---
def run_unilink_udp(label, strategy, threshold_mb, use_fc):
    print(f"\n>>> [Unilink UDP {label}] Starting...")
    sent_bytes = 0
    recv_bytes = 0
    bp_events = 0
    running = True
    send_allowed = True
    muted = False

    cfg_s = unilink.UdpConfig()
    cfg_s.local_port = PORT + 1
    peer_s = unilink.UdpClient(cfg_s)
    def _on_data(ctx):
        nonlocal recv_bytes
        if not muted: recv_bytes += len(ctx.data)
    peer_s.on_data(_on_data)
    peer_s.start_sync()

    cfg_c = unilink.UdpConfig()
    cfg_c.local_port = PORT + 2
    cfg_c.remote_address = "127.0.0.1"
    cfg_c.remote_port = PORT + 1
    peer_c = unilink.UdpClient(cfg_c)
    peer_c.backpressure_threshold = int(threshold_mb * 1024 * 1024)
    peer_c.backpressure_strategy = strategy
    
    def on_bp(queued):
        nonlocal bp_events, send_allowed
        limit = threshold_mb * 1024 * 1024
        if queued > limit * 0.8:
            bp_events += 1
            if use_fc: send_allowed = False
        elif queued < limit * 0.2:
            if use_fc: send_allowed = True
    peer_c.on_backpressure(on_bp)
    peer_c.start_sync()

    def chaos():
        nonlocal muted
        while running:
            time.sleep(CHAOS_INTERVAL)
            muted = True
            time.sleep(DOWN_TIME)
            muted = False
    threading.Thread(target=chaos, daemon=True).start()

    payload = b"A" * PAYLOAD_SIZE
    start_time = time.time()
    while time.time() - start_time < DURATION:
        if send_allowed:
            if peer_c.send(payload):
                sent_bytes += PAYLOAD_SIZE
        if SEND_SLEEP > 0: time.sleep(SEND_SLEEP)
    
    running = False
    time.sleep(0.5)
    peer_c.stop(); peer_s.stop()

    sent_mb = sent_bytes / (1024 * 1024)
    recv_mb = recv_bytes / (1024 * 1024)
    delivery = (recv_mb / sent_mb * 100) if sent_mb > 0 else 0
    print(f"    Sent: {sent_mb:.2f} MB | Recv: {recv_mb:.2f} MB | Delivery: {delivery:.2f}% | BP: {bp_events}")
    return delivery, sent_mb / DURATION

if __name__ == "__main__":
    print("=" * 70)
    print("Unilink UDP Comprehensive Benchmark (LV3 Chaos)")
    print("=" * 70)
    
    # 1. Raw UDP
    raw_res = run_raw_udp()
    
    # 2. KeepAll + FC
    all_res = run_unilink_udp("KeepAll+FC", unilink.BackpressureStrategy.KeepAll, 8, True)
    
    # 3. KeepLatest (No FC)
    lat_res = run_unilink_udp("KeepLatest (No FC)", unilink.BackpressureStrategy.KeepLatest, 0.5, False)

    print("\n" + "=" * 70)
    print(f"{'Metric':30s} {'Raw UDP':>12s} {'KeepAll+FC':>12s} {'KeepLatest':>12s}")
    print("-" * 70)
    print(f"{'Delivery Rate (%)':30s} {raw_res[0]:>12.2f} {all_res[0]:>12.2f} {lat_res[0]:>12.2f}")
    print(f"{'Throughput Avg (MB/s)':30s} {raw_res[1]:>12.2f} {all_res[1]:>12.2f} {lat_res[1]:>12.2f}")
    print("=" * 70)
