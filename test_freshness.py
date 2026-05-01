import sys
import os
import time

sys.path.append(os.path.join(os.getcwd(), "build/lib"))
sys.path.append(os.path.join(os.getcwd(), "build/bindings/python"))
import unilink_py as unilink

print("--- Testing BestEffort Recovery & Freshness ---")

PORT = 10090
server = unilink.TcpServer(PORT)
received_messages = []

def on_data(ctx):
    msg = bytes(ctx.data).decode('ascii', errors='ignore')
    received_messages.append(msg)

server.on_data(on_data)
server.start_sync()

client = unilink.TcpClient("127.0.0.1", PORT)
client.backpressure_strategy = unilink.BackpressureStrategy.BestEffort
# 512KB default
client.start_sync()
print("1. Connected.")

# --- Step 2: Disconnect ---
print("2. Simulating network failure (Stopping server)...")
server.stop()
time.sleep(1)

# --- Step 3: Flood with OLD data ---
print("3. Sending 1000 'OLD' messages...")
for i in range(1000):
    client.send(f"OLD_{i:04d}")

# --- Step 4: Send the LATEST data ---
print("4. Sending 5 'LATEST' messages...")
for i in range(5):
    client.send(f"LATEST_{i}")

# --- Step 5: Restore ---
print("5. Restarting server...")
server.start_sync()

# --- Step 6: Wait and Check ---
print("6. Waiting for recovery...")
time.sleep(3)

print(f"\nResults:")
print(f"Total messages received: {len(received_messages)}")
if len(received_messages) > 0:
    print(f"First received: {received_messages[0]}")
    print(f"Last received:  {received_messages[-1]}")
    
    # Check if 'LATEST' is present
    has_latest = any("LATEST" in m for m in received_messages)
    if has_latest:
        print("SUCCESS: Latest data received!")
    else:
        print("FAILURE: Latest data missing!")
else:
    print("FAILURE: No data received at all!")

client.stop()
server.stop()
