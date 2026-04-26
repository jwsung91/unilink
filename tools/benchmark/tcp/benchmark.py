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

def run_unilink_tcp_pingpong_zerocopy(num_pings):
    server = unilink.TcpServer(TCP_PORT)
    client = unilink.TcpClient(HOST, TCP_PORT)
    done = threading.Event()
    completed = 0

    def on_server_data(ctx):
        # Using memoryview to access data without copy
        server.send_to(ctx.client_id, memoryview(ctx))

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
    done = threading.Event()

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
                    if received >= total_bytes:
                        done.set()
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
        success = done.wait(timeout=max(10.0, num_chunks * 0.05))
        end_time = time.time()
        return (end_time - start_time) if success else -1
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

def run_unilink_tcp_pingpong_batch(num_pings):
    server = unilink.TcpServer(TCP_PORT)
    client = unilink.TcpClient(HOST, TCP_PORT)
    done = threading.Event()
    completed = 0

    def on_server_batch(batch):
        # Process a batch of messages in one GIL acquisition
        for ctx in batch:
            server.send_to(ctx.client_id, memoryview(ctx))

    def on_client_data(ctx):
        nonlocal completed
        completed += 1
        if completed < num_pings: client.send(PING_MESSAGE)
        else: done.set()

    server.on_data_batch(on_server_batch)
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

def run_unilink_tcp_throughput_batch(num_chunks):
    total_bytes = num_chunks * CHUNK_SIZE
    server = unilink.TcpServer(TCP_PORT)
    client = unilink.TcpClient(HOST, TCP_PORT)
    done = threading.Event()
    received_bytes = 0

    def on_server_batch(batch):
        nonlocal received_bytes
        for ctx in batch:
            received_bytes += len(ctx.data)
        if received_bytes >= total_bytes:
            done.set()

    server.on_data_batch(on_server_batch)
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

def _median(fn, *args, runs=3):
    results = []
    for _ in range(runs):
        t = fn(*args)
        if t > 0:
            results.append(t)
        time.sleep(0.1)
    if not results:
        return -1
    results.sort()
    return results[len(results) // 2]


def main():
    # Note: Python socket uses synchronous blocking I/O; unilink uses an async
    # callback model with internal worker threads. This benchmark compares
    # real-world latency/throughput under each model, not raw socket speed.
    ping_loads = [100, 500, 1000]
    chunk_loads = [1000, 5000, 10000]

    print("=== TCP Benchmark (Optimized Architecture) ===")
    print(f"{'Messages':<10} | {'Python (sec)':<15} | {'Unilink (sec)':<15} | {'Batching (sec)':<15}")
    for load in ping_loads:
        t_py = _median(run_python_tcp_pingpong, load)
        t_uni = _median(run_unilink_tcp_pingpong_zerocopy, load)
        t_batch = _median(run_unilink_tcp_pingpong_batch, load)

        res_py = f"{t_py:.4f}" if t_py > 0 else "FAIL"
        res_uni = f"{t_uni:.4f}" if t_uni > 0 else "FAIL"
        res_batch = f"{t_batch:.4f}" if t_batch > 0 else "FAIL"
        print(f"{load:<10} | {res_py:<15} | {res_uni:<15} | {res_batch:<15}")

    print("\n=== TCP Throughput (1KB Chunks) ===")
    print(f"{'Chunks':<10} | {'Python (sec)':<15} | {'Unilink (sec)':<15} | {'Batching (sec)':<15}")
    for load in chunk_loads:
        t_py = _median(run_python_tcp_throughput, load)
        t_uni = _median(run_unilink_tcp_throughput, load)
        t_batch = _median(run_unilink_tcp_throughput_batch, load)

        res_py = f"{t_py:.4f}" if t_py > 0 else "FAIL"
        res_uni = f"{t_uni:.4f}" if t_uni > 0 else "FAIL"
        res_batch = f"{t_batch:.4f}" if t_batch > 0 else "FAIL"
        print(f"{load:<10} | {res_py:<15} | {res_uni:<15} | {res_batch:<15}")

if __name__ == "__main__":
    main()
