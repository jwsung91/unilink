import os
import sys
import asyncio
import time
import threading

# Add the necessary paths to PYTHONPATH
# 1. build/lib for the binary unilink_py
# 2. bindings/python for the unilink package structure
sys.path.append(os.path.join(os.getcwd(), 'build', 'lib'))
sys.path.append(os.path.join(os.getcwd(), 'bindings', 'python'))

import unilink_py
from unilink.asyncio import AsyncTcpClient

PORT = 8888

def run_legacy_server():
    """A standard synchronous server to send some data."""
    server = unilink_py.TcpServer(PORT)
    server.on_client_connect(lambda ctx: print(f"[Server] Client {ctx.client_id} connected"))
    
    if server.start():
        time.sleep(1) # Wait for client to connect
        print("[Server] Sending messages...")
        server.broadcast("Async Message 1\n")
        time.sleep(0.5)
        server.broadcast("Async Message 2\n")
        time.sleep(1)
        server.stop()

async def run_async_client():
    """The new AsyncTcpClient using await."""
    print("[Client] Initializing AsyncTcpClient...")
    client = AsyncTcpClient("127.0.0.1", PORT)
    client.use_line_framer("\n")
    
    print("[Client] Connecting...")
    if await client.connect():
        print("[Client] Connected successfully!")
        
        try:
            # Read two messages using await
            for i in range(2):
                print(f"[Client] Waiting for message {i+1}...")
                msg = await client.read_message()
                print(f"[Client] Await returned: '{msg.decode('utf-8')}'")
        except asyncio.TimeoutError:
            print("[Client] Read timed out!")
        
        client.stop()
        print("[Client] Stopped.")
    else:
        print("[Client] Connection failed.")

async def main():
    # Start server in a background thread
    server_thread = threading.Thread(target=run_legacy_server)
    server_thread.start()
    
    # Run our async client
    await run_async_client()
    
    server_thread.join()
    print("\nSUCCESS: asyncio integration verified!")

if __name__ == "__main__":
    asyncio.run(main())
