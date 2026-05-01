import sys
import os
import time

sys.path.append(os.path.join(os.getcwd(), "build/lib"))
sys.path.append(os.path.join(os.getcwd(), "build/bindings/python"))
import unilink_py as unilink

print("--- Testing BestEffort Drops (Corrected Logic) ---")

PORT = 10085
server = unilink.TcpServer(PORT)
recv_count = 0
def on_data(ctx):
    global recv_count
    recv_count += len(ctx.data)
server.on_data(on_data)
server.start_sync()

client = unilink.TcpClient("127.0.0.1", PORT)
client.backpressure_strategy = unilink.BackpressureStrategy.BestEffort
# No threshold set -> should use 512KB default
client.start_sync()
print("1. Client/Server Connected.")

# --- Simulate Network Down ---
print("2. Stopping server to simulate congestion...")
server.stop()
time.sleep(1) 

# --- Send Flood ---
payload = b"X" * 1024 # 1KB
print("3. Sending 2000 x 1KB messages while server is down...")
for i in range(2000):
    client.send(payload)

# --- Restore ---
print("4. Restarting server...")
server.start_sync()

print("5. Waiting for queue to drain...")
time.sleep(3) # Give more time for BestEffort to send what's left

print(f"Results: Total sent: 2000 KB, Total received: {recv_count / 1024:.2f} KB")

# Threshold is 512KB. bp_limit is 4*512KB = 2MB.
# Since it's BestEffort, it should keep only the LATEST ~512KB.
if recv_count < 1500: # We expect around 512KB ~ 600KB
    print(f"SUCCESS: Data was dropped. (Recv: {recv_count/1024:.2f} KB < Sent: 2000 KB)")
else:
    print(f"FAILURE: Too much data received. (Recv: {recv_count/1024:.2f} KB)")

client.stop()
server.stop()
