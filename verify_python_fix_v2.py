import sys
import os
import time
from datetime import timedelta

sys.path.append(os.path.join(os.getcwd(), "build/lib"))
sys.path.append(os.path.join(os.getcwd(), "build/bindings/python"))
import unilink_py as unilink

print("--- [FINAL VERIFICATION] BestEffort Recovery Test (v2) ---")

PORT = 10102
server = unilink.TcpServer(PORT)
received = []

def on_data(ctx):
    msg = bytes(ctx.data).decode('ascii', errors='ignore')
    received.append(msg)

server.on_data(on_data)
server.start_sync()

client = unilink.TcpClient("127.0.0.1", PORT)
client.backpressure_strategy = unilink.BackpressureStrategy.BestEffort
# Fix: retry_interval takes timedelta
client.retry_interval(timedelta(milliseconds=100)) 
client.start_sync()
print("1. Initial Connection Established.")

# --- Disconnect ---
print("2. Simulating failure (Stopping server)...")
server.stop()
time.sleep(1) # Longer sleep to ensure disconnect is processed

# --- Send stale and latest data ---
print("3. Flooding with data while disconnected...")
# Since threshold is 512KB, 100 messages of 1KB will fit, but let's test the logic.
for i in range(100):
    client.send(f"OLD_{i:03d}".encode('ascii'))

client.send(b"THE_LATEST_FRAME") # This one MUST survive
print("   -> Sent 100 OLD frames and 1 THE_LATEST_FRAME.")

# --- Restore ---
print("4. Restarting server...")
server.start_sync()

# --- Polling Wait ---
print("5. Waiting for reconnection and data flush (max 5s)...")
success = False
for _ in range(50):
    if any("THE_LATEST_FRAME" in m for m in received):
        success = True
        break
    time.sleep(0.1)

print(f"\nResults:")
print(f"Total messages received: {len(received)}")
if success:
    print("✅ SUCCESS: The latest frame was recovered and received after reconnection!")
    old_count = sum(1 for m in received if "OLD" in m)
    print(f"   (Recovered {old_count} old frames and the latest one)")
else:
    print("❌ FAILURE: The latest frame was lost.")
    if len(received) > 0:
        print(f"   Note: Received {len(received)} messages, but none were the latest.")

client.stop()
server.stop()
