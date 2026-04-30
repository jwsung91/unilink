import sys
import os
import time

sys.path.append(os.path.join(os.getcwd(), "build/lib"))
sys.path.append(os.path.join(os.getcwd(), "build/bindings/python"))
import unilink_py as unilink

print("--- Testing BestEffort Drops with New Default (512KB) ---")

PORT = 10082
server = unilink.TcpServer(PORT)
recv_count = 0
def on_data(ctx):
    global recv_count
    recv_count += len(ctx.data)
server.on_data(on_data)
server.start_sync()

client = unilink.TcpClient("127.0.0.1", PORT)
client.backpressure_strategy = unilink.BackpressureStrategy.BestEffort
# We DON'T set backpressure_threshold here. 
# It should be DEFAULT_BACKPRESSURE_THRESHOLD (512KB).
client.start_sync()

# Send many small messages to fill up the queue
# We want to exceed 512KB.
payload = b"X" * 1024 # 1KB
print("Sending 1000 x 1KB messages...")
for _ in range(1000):
    client.send(payload)

time.sleep(1.0)
print(f"Received by server: {recv_count / 1024} KB")

# If threshold is 512KB, and it's dropping oldest, 
# and the receiver is consuming, some might still get through.
# But if we send fast enough, it should drop.
# Actually, the receiver IS consuming. To test drop, we should STOP the server.

print("\n--- Testing with server STOPPED ---")
client.stop()
server.stop()
recv_count = 0

server = unilink.TcpServer(PORT)
server.on_data(on_data)
# DON'T start server yet

client = unilink.TcpClient("127.0.0.1", PORT)
client.backpressure_strategy = unilink.BackpressureStrategy.BestEffort
client.start_sync() # Will try to connect, but fail or wait? 
# Actually TcpClient tries to connect in background. 

print("Sending 2000 x 1KB messages while disconnected...")
for i in range(2000):
    client.send(payload)
    if i % 500 == 0: print(f"  Sent {i}...")

print("Starting server now...")
server.start_sync()

time.sleep(2.0)
print(f"Received by server: {recv_count / 1024} KB")

if recv_count < 1500 * 1024: # Should be around 512KB
    print(f"SUCCESS: Data was dropped. Recv: {recv_count/1024} KB (Expected < 2000 KB)")
else:
    print(f"FAILURE: Too much data received. Recv: {recv_count/1024} KB")

client.stop()
server.stop()
