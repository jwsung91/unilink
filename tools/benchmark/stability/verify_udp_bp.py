"""
Unilink UDP LV4 Backpressure Verification
=========================================================
Final attempt at verification using a non-blocking sender 
and a toggled reader to ensure queue buildup.
"""

import time
import threading
import sys
import os

sys.path.append(os.path.join(os.getcwd(), "build/lib"))
sys.path.append(os.path.join(os.getcwd(), "build/bindings/python"))
import unilink_py as unilink

PAYLOAD_SIZE    = 4096
SEND_SLEEP      = 0.00001    # 10us
DURATION        = 10
PORT            = 10110
THRESHOLD_KB    = 16         # Very small to trigger BP quickly

def run_bench(label, strategy, use_fc):
    print(f"\n>>> [{label}] strategy={strategy}")
    sent_bytes = 0
    recv_bytes = 0
    bp_events = 0
    running = True
    reader_active = True
    send_allowed = True

    # 1. Receiver
    cfg_s = unilink.UdpConfig()
    cfg_s.local_port = PORT
    peer_s = unilink.UdpClient(cfg_s)
    def _count(ctx):
        nonlocal recv_bytes
        if reader_active: recv_bytes += len(ctx.data)
    peer_s.on_data(_count)
    peer_s.start_sync()

    # 2. Sender
    cfg_c = unilink.UdpConfig()
    cfg_c.local_port = PORT + 1
    cfg_c.remote_address = "127.0.0.1"
    cfg_c.remote_port = PORT
    peer_c = unilink.UdpClient(cfg_c)
    peer_c.backpressure_threshold = THRESHOLD_KB * 1024
    peer_c.backpressure_strategy = strategy
    
    def on_bp(q):
        nonlocal bp_events, send_allowed
        bp_events += 1
        if use_fc: send_allowed = (q < THRESHOLD_KB * 1024 * 0.2)
    peer_c.on_backpressure(on_bp)
    peer_c.start_sync()

    # 3. Chaos Loop (Reader Pause)
    def chaos():
        nonlocal reader_active
        while running:
            time.sleep(2)
            # In UDP on loopback, if we don't read, the OS buffer fills up.
            # If the OS buffer is full, Unilink C++ queue starts to grow.
            reader_active = False 
            time.sleep(2)
            reader_active = True
    threading.Thread(target=chaos, daemon=True).start()

    # 4. Sender Loop
    payload = b"A" * PAYLOAD_SIZE
    start_time = time.time()
    while time.time() - start_time < DURATION:
        if send_allowed:
            if peer_c.send(payload):
                sent_bytes += PAYLOAD_SIZE
        time.sleep(SEND_SLEEP)
    
    running = False
    time.sleep(0.5)
    peer_c.stop(); peer_s.stop()

    sent_mb = sent_bytes / (1024 * 1024)
    recv_mb = recv_bytes / (1024 * 1024)
    delivery = (recv_mb / sent_mb * 100) if sent_mb > 0 else 0
    print(f"    Sent: {sent_mb:.2f} MB | Recv: {recv_mb:.2f} MB | Delivery: {delivery:.2f}% | BP: {bp_events}")

if __name__ == "__main__":
    print("=" * 70)
    print("Unilink UDP Backpressure Verification (LV4 Load)")
    print("=" * 70)
    run_bench("Reliable + Flow Control", unilink.BackpressureStrategy.Reliable, True)
    run_bench("BestEffort (No Flow Control)", unilink.BackpressureStrategy.BestEffort, False)
