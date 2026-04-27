"""
Unilink UDP LV4 Extreme Stress Benchmark
=========================================================
Forces backpressure by stopping receiver I/O and sending at max speed.
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

# LV4 Extreme Spec
PAYLOAD_SIZE    = 4096
SEND_SLEEP      = 0          # Max speed
DURATION        = 15
PORT            = 10100
CHAOS_INTERVAL  = 2.0        # 2s up
DOWN_TIME       = 2.0        # 2s down
THRESHOLD_KB    = 32         # Tiny threshold to force BP

def run_unilink_udp(label, strategy, use_fc):
    print(f"\n>>> [Unilink UDP {label}] Threshold={THRESHOLD_KB}KB, Strategy={strategy}")
    sent_bytes = 0
    recv_bytes = 0
    bp_events = 0
    running = True
    send_allowed = True
    network_active = True

    # --- Receiver (Peer S) ---
    cfg_s = unilink.UdpConfig()
    cfg_s.local_port = PORT + 1
    peer_s = unilink.UdpClient(cfg_s)
    
    def _on_data(ctx):
        nonlocal recv_bytes
        # In this test, we don't mute here; chaos_monkey will stop the peer entirely
        recv_bytes += len(ctx.data)
    peer_s.on_data(_on_data)
    peer_s.start_sync()

    # --- Sender (Peer C) ---
    cfg_c = unilink.UdpConfig()
    cfg_c.local_port = PORT + 2
    cfg_c.remote_address = "127.0.0.1"
    cfg_c.remote_port = PORT + 1
    peer_c = unilink.UdpClient(cfg_c)
    peer_c.backpressure_threshold = THRESHOLD_KB * 1024
    peer_c.backpressure_strategy = strategy
    
    def on_bp(queued):
        nonlocal bp_events, send_allowed
        if queued > (THRESHOLD_KB * 1024 * 0.8):
            bp_events += 1
            if use_fc: send_allowed = False
        elif queued < (THRESHOLD_KB * 1024 * 0.2):
            if use_fc: send_allowed = True
    peer_c.on_backpressure(on_bp)
    peer_c.start_sync()

    # --- Chaos Monkey (Hard Stop/Start I/O) ---
    def chaos_monkey():
        nonlocal network_active
        while running:
            time.sleep(CHAOS_INTERVAL)
            if not running: break
            # print("    [Chaos] STOPPING Receiver I/O...")
            peer_s.stop() # This will cause sender's buffers to fill
            network_active = False
            time.sleep(DOWN_TIME)
            if not running: break
            # print("    [Chaos] RESTARTING Receiver I/O...")
            peer_s.start_sync()
            network_active = True
            
    threading.Thread(target=chaos_monkey, daemon=True).start()

    # --- Sender Loop ---
    payload = b"A" * PAYLOAD_SIZE
    start_time = time.time()
    while time.time() - start_time < DURATION:
        if send_allowed:
            if peer_c.send(payload):
                sent_bytes += PAYLOAD_SIZE
        
        # In extreme LV4, we still need to yield the GIL 
        # so chaos/monitor threads can actually function
        time.sleep(0.000001) # 1us yield
    
    running = False
    time.sleep(0.5)
    peer_c.stop(); peer_s.stop()

    sent_mb = sent_bytes / (1024 * 1024)
    recv_mb = recv_bytes / (1024 * 1024)
    delivery = (recv_mb / sent_mb * 100) if sent_mb > 0 else 0
    print(f"    Sent: {sent_mb:.2f} MB | Recv: {recv_mb:.2f} MB | Delivery: {delivery:.2f}% | BP: {bp_events}")
    return delivery, sent_mb / DURATION, bp_events

if __name__ == "__main__":
    print("=" * 75)
    print(f"Unilink UDP LV4 Extreme Benchmark (Duration: {DURATION}s)")
    print("=" * 75)
    
    # 1. KeepAll + Flow Control
    all_res = run_unilink_udp("KeepAll + FC", unilink.BackpressureStrategy.KeepAll, True)
    
    # 2. KeepLatest (No Flow Control)
    lat_res = run_unilink_udp("KeepLatest (No FC)", unilink.BackpressureStrategy.KeepLatest, False)

    print("\n" + "=" * 75)
    print(f"{'Metric':30s} {'KeepAll + FC':>18s} {'KeepLatest':>20s}")
    print("-" * 75)
    print(f"{'Delivery Rate (%)':30s} {all_res[0]:>18.2f} {lat_res[0]:>20.2f}")
    print(f"{'Throughput Avg (MB/s)':30s} {all_res[1]:>18.2f} {lat_res[1]:>20.2f}")
    print(f"{'Backpressure Events':30s} {all_res[2]:>18.0f} {lat_res[2]:>20.0f}")
    print("=" * 75)
