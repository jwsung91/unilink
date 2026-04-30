import sys
import os
import time

sys.path.append(os.path.join(os.getcwd(), "build/lib"))
sys.path.append(os.path.join(os.getcwd(), "build/bindings/python"))
import unilink_py as unilink

print("--- Testing BestEffort Drops with New Default (512KB) ---")

PORT = 10080
server = unilink.TcpServer(PORT)
recv_count = 0
def on_data(ctx):
    global recv_count
    recv_count += len(ctx.data)
server.on_data(on_data)
server.start_sync()

client = unilink.TcpClient("127.0.0.1", PORT)
client.backpressure_strategy = unilink.BackpressureStrategy.BestEffort
# We DON'T set backpressure_threshold here. It should use 512KB.
client.start_sync()

# Send data fast while receiver is NOT consuming (we'll just use a large payload)
# 1MB payload should trigger drop if threshold is 512KB
payload = b"X" * (1024 * 1024) 
print("Sending 1MB payload (BestEffort, Default threshold)...")
client.send(payload)

time.sleep(0.5)
print(f"Received by server: {recv_count / 1024} KB")

if recv_count < 1024 * 1024:
    print("SUCCESS: Data was dropped (Threshold < 1MB)")
else:
    print("FAILURE: All data received (Threshold >= 1MB)")

client.stop()
server.stop()
