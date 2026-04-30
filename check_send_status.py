import sys
import os
import time

sys.path.append(os.path.join(os.getcwd(), "build/lib"))
sys.path.append(os.path.join(os.getcwd(), "build/bindings/python"))
import unilink_py as unilink

print("--- Checking if Python send() works while disconnected ---")

PORT = 10095
server = unilink.TcpServer(PORT)
server.start_sync()

client = unilink.TcpClient("127.0.0.1", PORT)
client.start_sync()
print("1. Initial connection OK.")

server.stop()
time.sleep(0.5)
print(f"2. Server stopped. Client connected() status: {client.connected()}")

res = client.send(b"HELLO_DURING_DISCONNECT")
print(f"3. send() result while disconnected: {res}")

client.stop()
server.stop()
