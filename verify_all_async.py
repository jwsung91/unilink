import os
import sys
import asyncio
import time
import threading

# Add necessary paths
sys.path.append(os.path.join(os.getcwd(), 'build', 'lib'))
sys.path.append(os.path.join(os.getcwd(), 'bindings', 'python'))

import unilink_py
from unilink.asyncio import AsyncTcpClient, AsyncUdsClient, AsyncUdp

PORT_TCP = 9991
PORT_UDP = 9992
UDS_PATH = "/tmp/async_test.sock"
TIMEOUT = 5.0 # 5 seconds timeout for each step

async def test_tcp():
    print("\n--- Testing AsyncTcpClient ---")
    server = unilink_py.TcpServer(PORT_TCP)
    if not server.start():
        print("TCP Server failed to start")
        return

    client = AsyncTcpClient("127.0.0.1", PORT_TCP)
    try:
        if await asyncio.wait_for(client.connect(), timeout=TIMEOUT):
            print("TCP Connected")
            client.use_line_framer("\n")
            
            # Wait a bit for setup
            await asyncio.sleep(0.5)
            server.broadcast("TCP Hello\n")
            
            msg = await asyncio.wait_for(client.read_message(), timeout=TIMEOUT)
            print(f"TCP Received: {msg.decode()}")
        else:
            print("TCP Connection failed (returned False)")
    except asyncio.TimeoutError:
        print("TCP Test Timed Out!")
    finally:
        client.stop()
        server.stop()

async def test_uds():
    print("\n--- Testing AsyncUdsClient ---")
    if os.path.exists(UDS_PATH): os.remove(UDS_PATH)
    server = unilink_py.UdsServer(UDS_PATH)
    if not server.start():
        print("UDS Server failed to start")
        return
    
    client = AsyncUdsClient(UDS_PATH)
    try:
        if await asyncio.wait_for(client.connect(), timeout=TIMEOUT):
            print("UDS Connected")
            client.use_line_framer("\n")
            
            await asyncio.sleep(0.5)
            server.broadcast("UDS Hello\n")
            
            msg = await asyncio.wait_for(client.read_message(), timeout=TIMEOUT)
            print(f"UDS Received: {msg.decode()}")
        else:
            print("UDS Connection failed")
    except asyncio.TimeoutError:
        print("UDS Test Timed Out!")
    finally:
        client.stop()
        server.stop()
        if os.path.exists(UDS_PATH): os.remove(UDS_PATH)

async def test_udp():
    print("\n--- Testing AsyncUdp ---")
    rx_cfg = unilink_py.UdpConfig()
    rx_cfg.local_address = "127.0.0.1"
    rx_cfg.local_port = PORT_UDP
    
    # Direct callback to avoid any queue issues
    received_raw = []
    def on_raw_data(ctx):
        print(f"[UDP-RAW] RECEIVED: {ctx.data}")
        received_raw.append(ctx.data)
        
    receiver = unilink_py.Udp(rx_cfg)
    receiver.on_data(on_raw_data)
    
    print("[UDP] Starting receiver...")
    if not receiver.start():
        print("[UDP] Receiver failed to start")
        return
    
    tx_cfg = unilink_py.UdpConfig()
    tx_cfg.remote_address = "127.0.0.1"
    tx_cfg.remote_port = PORT_UDP
    client = AsyncUdp(tx_cfg)
    
    try:
        print("[UDP] Client connecting...")
        await asyncio.wait_for(client.connect(), timeout=TIMEOUT)
        
        print("[UDP] Client sending 'UDP Hello'...")
        client.send("UDP Hello")
        
        # Wait and see if callback fires
        print("[UDP] Waiting for raw callback...")
        for _ in range(20):
            if received_raw:
                print(f"[UDP] SUCCESS! Data found in raw list: {received_raw[0]}")
                break
            await asyncio.sleep(0.1)
        else:
            print("[UDP] FAILED: Raw callback never fired.")
            
    finally:
        client.stop()
        receiver.stop()

async def main():
    try:
        await test_tcp()
        await test_uds()
        await test_udp()
        print("\nALL ASYNC CLASSES VERIFIED!")
    except Exception as e:
        print(f"\nUnexpected error during verification: {e}")

if __name__ == "__main__":
    asyncio.run(main())
