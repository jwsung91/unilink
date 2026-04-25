import socket
import time
import threading
import unilink
import sys

PING_MESSAGE = b"PING"
PING_LEN = len(PING_MESSAGE)
CHUNK_SIZE = 1024
CHUNK_MESSAGE = b"A" * CHUNK_SIZE
HOST = "127.0.0.1"
UDP_PORT_S = 9998
UDP_PORT_C = 9997

def run_python_udp_pingpong(num_pings):
    def server_thread():
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                s.bind((HOST, UDP_PORT_S))
                s.settimeout(5.0)
                for _ in range(num_pings):
                    data, addr = s.recvfrom(4096)
                    s.sendto(data, addr)
        except: pass

    t = threading.Thread(target=server_thread, daemon=True)
    t.start()
    time.sleep(0.2)

    start_time = time.time()
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as c:
            c.settimeout(2.0)
            for _ in range(num_pings):
                c.sendto(PING_MESSAGE, (HOST, UDP_PORT_S))
                data, addr = c.recvfrom(4096)
        return time.time() - start_time
    except:
        return -1

def run_unilink_udp_pingpong(num_pings):
    cfg_s = unilink.UdpConfig(); cfg_s.local_port = UDP_PORT_S; cfg_s.remote_address = HOST; cfg_s.remote_port = UDP_PORT_C
    cfg_c = unilink.UdpConfig(); cfg_c.local_port = UDP_PORT_C; cfg_c.remote_address = HOST; cfg_c.remote_port = UDP_PORT_S
    
    server = unilink.UdpClient(cfg_s)
    client = unilink.UdpClient(cfg_c)
    done = threading.Event()
    completed = 0

    def on_server_data(ctx): server.send(ctx.data)
    def on_client_data(ctx):
        nonlocal completed
        completed += 1
        if completed < num_pings: client.send(PING_MESSAGE)
        else: done.set()

    server.on_data(on_server_data); client.on_data(on_client_data)
    if not server.start() or not client.start():
        server.stop(); client.stop()
        return -1
    
    start_time = time.time()
    client.send(PING_MESSAGE)
    success = done.wait(timeout=max(5.0, num_pings * 0.05))
    end_time = time.time()
    
    client.stop(); server.stop()
    return (end_time - start_time) if success else -1

def run_unilink_udp_pingpong_zerocopy(num_pings):
    cfg_s = unilink.UdpConfig(); cfg_s.local_port = UDP_PORT_S; cfg_s.remote_address = HOST; cfg_s.remote_port = UDP_PORT_C
    cfg_c = unilink.UdpConfig(); cfg_c.local_port = UDP_PORT_C; cfg_c.remote_address = HOST; cfg_c.remote_port = UDP_PORT_S
    
    server = unilink.UdpClient(cfg_s)
    client = unilink.UdpClient(cfg_c)
    done = threading.Event()
    completed = 0

    def on_server_data(ctx): server.send(memoryview(ctx))
    def on_client_data(ctx):
        nonlocal completed
        completed += 1
        if completed < num_pings: client.send(PING_MESSAGE)
        else: done.set()

    server.on_data(on_server_data); client.on_data(on_client_data)
    if not server.start() or not client.start():
        server.stop(); client.stop()
        return -1
    
    start_time = time.time()
    client.send(PING_MESSAGE)
    success = done.wait(timeout=max(5.0, num_pings * 0.05))
    end_time = time.time()
    
    client.stop(); server.stop()
    return (end_time - start_time) if success else -1

def run_python_udp_throughput(num_chunks):
    total_bytes = num_chunks * CHUNK_SIZE
    def server_thread():
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                s.bind((HOST, UDP_PORT_S))
                s.settimeout(10.0)
                received = 0
                while received < total_bytes:
                    data, addr = s.recvfrom(8192)
                    received += len(data)
        except: pass

    t = threading.Thread(target=server_thread, daemon=True)
    t.start()
    time.sleep(0.2)

    start_time = time.time()
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as c:
            for i in range(num_chunks):
                c.sendto(CHUNK_MESSAGE, (HOST, UDP_PORT_S))
                if i % 10 == 0: time.sleep(0.001)
        t.join(timeout=5.0)
        return time.time() - start_time
    except:
        return -1

def run_unilink_udp_throughput(num_chunks):
    total_bytes = num_chunks * CHUNK_SIZE
    cfg_s = unilink.UdpConfig(); cfg_s.local_port = UDP_PORT_S; cfg_s.remote_address = HOST; cfg_s.remote_port = UDP_PORT_C
    cfg_c = unilink.UdpConfig(); cfg_c.local_port = UDP_PORT_C; cfg_c.remote_address = HOST; cfg_c.remote_port = UDP_PORT_S

    server = unilink.UdpClient(cfg_s)
    client = unilink.UdpClient(cfg_c)
    done = threading.Event()
    received_bytes = 0

    def on_server_data(ctx):
        nonlocal received_bytes
        received_bytes += len(ctx.data)
        if received_bytes >= total_bytes: done.set()

    server.on_data(on_server_data)
    if not server.start() or not client.start():
        server.stop(); client.stop()
        return -1
    
    start_time = time.time()
    for i in range(num_chunks):
        client.send(CHUNK_MESSAGE)
        if i % 10 == 0: time.sleep(0.001)
    
    success = done.wait(timeout=max(15.0, num_chunks * 0.1))
    end_time = time.time()
    
    client.stop(); server.stop()
    return (end_time - start_time) if success else -1

def main():
    ping_loads = [100, 500, 1000]
    chunk_loads = [100, 500, 1000]

    print("=== UDP Benchmark (Timeout enabled) ===")
    print(f"{'Messages':<10} | {'Python (sec)':<15} | {'Unilink (sec)':<15} | {'ZeroCopy (sec)':<15}")
    for load in ping_loads:
        t_py = run_python_udp_pingpong(load)
        t_uni = run_unilink_udp_pingpong(load)
        t_zc = run_unilink_udp_pingpong_zerocopy(load)
        
        res_py = f"{t_py:.4f}" if t_py > 0 else "FAIL"
        res_uni = f"{t_uni:.4f}" if t_uni > 0 else "FAIL"
        res_zc = f"{t_zc:.4f}" if t_zc > 0 else "FAIL"
        print(f"{load:<10} | {res_py:<15} | {res_uni:<15} | {res_zc:<15}")

    print("\n=== UDP Throughput (1KB Chunks) ===")
    for load in chunk_loads:
        t_py = run_python_udp_throughput(load)
        t_uni = run_unilink_udp_throughput(load)
        
        res_py = f"{t_py:.4f}" if t_py > 0 else "FAIL"
        res_uni = f"{t_uni:.4f}" if t_uni > 0 else "FAIL"
        print(f"{load:<10} | {res_py:<15} | {res_uni:<15}")

if __name__ == "__main__":
    main()
