import socket
import time
import threading
import unilink
import sys
import os

PING_MESSAGE = b"PING"
PING_LEN = len(PING_MESSAGE)
CHUNK_SIZE = 1024
CHUNK_MESSAGE = b"A" * CHUNK_SIZE
HOST = "127.0.0.1"
TCP_PORT = 9999

def run_python_tcp_pingpong(num_pings):
    def server_thread():
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s.bind((HOST, TCP_PORT))
                s.listen(1)
                s.settimeout(5.0)
                conn, addr = s.accept()
                with conn:
                    conn.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                    for _ in range(num_pings):
                        data = conn.recv(PING_LEN)
                        if not data: break
                        conn.sendall(data)
        except: pass

    t = threading.Thread(target=server_thread, daemon=True)
    t.start()
    time.sleep(0.2)

    start_time = time.time()
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as c:
            c.settimeout(2.0)
            c.connect((HOST, TCP_PORT))
            c.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            for _ in range(num_pings):
                c.sendall(PING_MESSAGE)
                data = c.recv(PING_LEN)
                if not data: break
        return time.time() - start_time
    except Exception as e:
        return -1

def run_unilink_tcp_pingpong(num_pings):
    server = unilink.TcpServer(TCP_PORT)
    client = unilink.TcpClient(HOST, TCP_PORT)
    done = threading.Event()
    completed = 0

    def on_server_data(ctx):
        server.send_to(ctx.client_id, ctx.data)
    def on_client_data(ctx):
        nonlocal completed
        completed += 1
        if completed < num_pings: client.send(PING_MESSAGE)
        else: done.set()

    server.on_data(on_server_data)
    client.on_data(on_client_data)
    
    if not server.start() or not client.start():
        server.stop(); client.stop()
        return -1
    
    start_time = time.time()
    client.send(PING_MESSAGE)
    success = done.wait(timeout=max(5.0, num_pings * 0.05))
    end_time = time.time()
    
    client.stop(); server.stop()
    return (end_time - start_time) if success else -1

def run_python_tcp_throughput(num_chunks):
    total_bytes = num_chunks * CHUNK_SIZE
    def server_thread():
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s.bind((HOST, TCP_PORT))
                s.listen(1)
                s.settimeout(5.0)
                conn, addr = s.accept()
                with conn:
                    received = 0
                    while received < total_bytes:
                        data = conn.recv(8192)
                        if not data: break
                        received += len(data)
        except: pass

    t = threading.Thread(target=server_thread, daemon=True)
    t.start()
    time.sleep(0.2)

    start_time = time.time()
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as c:
            c.settimeout(5.0)
            c.connect((HOST, TCP_PORT))
            for _ in range(num_chunks):
                c.sendall(CHUNK_MESSAGE)
        t.join(timeout=2.0)
        return time.time() - start_time
    except:
        return -1

def run_unilink_tcp_throughput(num_chunks):
    total_bytes = num_chunks * CHUNK_SIZE
    server = unilink.TcpServer(TCP_PORT)
    client = unilink.TcpClient(HOST, TCP_PORT)
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
    for _ in range(num_chunks):
        client.send(CHUNK_MESSAGE)
    
    success = done.wait(timeout=max(10.0, num_chunks * 0.05))
    end_time = time.time()
    
    client.stop(); server.stop()
    return (end_time - start_time) if success else -1

def main():
    ping_loads = [100, 500, 1000]
    chunk_loads = [100, 500, 1000]

    print("=== TCP Benchmark (Timeout enabled) ===")
    print(f"{'Messages':<10} | {'Python (sec)':<15} | {'Unilink (sec)':<15}")
    for load in ping_loads:
        t_py = run_python_tcp_pingpong(load)
        t_uni = run_unilink_tcp_pingpong(load)
        print(f"{load:<10} | {t_py if t_py > 0 else 'FAIL':<15.4f} | {t_uni if t_uni > 0 else 'FAIL':<15.4f}")

    print("\n=== TCP Throughput (1KB Chunks) ===")
    for load in chunk_loads:
        t_py = run_python_tcp_throughput(load)
        t_uni = run_unilink_tcp_throughput(load)
        print(f"{load:<10} | {t_py if t_py > 0 else 'FAIL':<15.4f} | {t_uni if t_uni > 0 else 'FAIL':<15.4f}")

if __name__ == "__main__":
    main()
