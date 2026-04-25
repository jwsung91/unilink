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
SOCKET_PATH = "/tmp/unilink_bench.sock"

def cleanup():
    if os.path.exists(SOCKET_PATH):
        try: os.remove(SOCKET_PATH)
        except: pass

def run_python_uds_pingpong(num_pings):
    cleanup()
    def server_thread():
        try:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
                s.bind(SOCKET_PATH)
                s.listen(1)
                s.settimeout(5.0)
                conn, addr = s.accept()
                with conn:
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
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as c:
            c.settimeout(2.0)
            c.connect(SOCKET_PATH)
            for _ in range(num_pings):
                c.sendall(PING_MESSAGE)
                data = c.recv(PING_LEN)
                if not data: break
        return time.time() - start_time
    except:
        return -1
    finally:
        cleanup()

def run_unilink_uds_pingpong(num_pings):
    cleanup()
    server = unilink.UdsServer(SOCKET_PATH)
    client = unilink.UdsClient(SOCKET_PATH)
    done = threading.Event()
    completed = 0

    def on_server_data(ctx): server.send_to(ctx.client_id, ctx.data)
    def on_client_data(ctx):
        nonlocal completed
        completed += 1
        if completed < num_pings: client.send(PING_MESSAGE)
        else: done.set()

    server.on_data(on_server_data); client.on_data(on_client_data)
    if not server.start() or not client.start():
        server.stop(); client.stop(); cleanup()
        return -1
    
    start_time = time.time()
    client.send(PING_MESSAGE)
    success = done.wait(timeout=max(5.0, num_pings * 0.05))
    end_time = time.time()
    
    client.stop(); server.stop(); cleanup()
    return (end_time - start_time) if success else -1

def run_unilink_uds_pingpong_zerocopy(num_pings):
    cleanup()
    server = unilink.UdsServer(SOCKET_PATH)
    client = unilink.UdsClient(SOCKET_PATH)
    done = threading.Event()
    completed = 0

    def on_server_data(ctx): server.send_to(ctx.client_id, memoryview(ctx))
    def on_client_data(ctx):
        nonlocal completed
        completed += 1
        if completed < num_pings: client.send(PING_MESSAGE)
        else: done.set()

    server.on_data(on_server_data); client.on_data(on_client_data)
    if not server.start() or not client.start():
        server.stop(); client.stop(); cleanup()
        return -1
    
    start_time = time.time()
    client.send(PING_MESSAGE)
    success = done.wait(timeout=max(5.0, num_pings * 0.05))
    end_time = time.time()
    
    client.stop(); server.stop(); cleanup()
    return (end_time - start_time) if success else -1

def run_python_uds_throughput(num_chunks):
    cleanup()
    total_bytes = num_chunks * CHUNK_SIZE
    def server_thread():
        try:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
                s.bind(SOCKET_PATH)
                s.listen(1)
                s.settimeout(10.0)
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
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as c:
            c.settimeout(5.0)
            c.connect(SOCKET_PATH)
            for _ in range(num_chunks):
                c.sendall(CHUNK_MESSAGE)
        t.join(timeout=5.0)
        return time.time() - start_time
    except:
        return -1
    finally:
        cleanup()

def run_unilink_uds_throughput(num_chunks):
    cleanup()
    total_bytes = num_chunks * CHUNK_SIZE
    server = unilink.UdsServer(SOCKET_PATH)
    client = unilink.UdsClient(SOCKET_PATH)
    done = threading.Event()
    received_bytes = 0

    def on_server_data(ctx):
        nonlocal received_bytes
        received_bytes += len(ctx.data)
        if received_bytes >= total_bytes: done.set()

    server.on_data(on_server_data)
    if not server.start() or not client.start():
        server.stop(); client.stop(); cleanup()
        return -1
    
    start_time = time.time()
    for i in range(num_chunks):
        client.send(CHUNK_MESSAGE)
        if i % 20 == 0: time.sleep(0.001) # Safety for UDS high load
    
    success = done.wait(timeout=max(10.0, num_chunks * 0.1))
    end_time = time.time()
    
    client.stop(); server.stop(); cleanup()
    return (end_time - start_time) if success else -1

def main():
    ping_loads = [500]
    chunk_loads = [100, 500, 1000]

    print("=== UDS Benchmark (Timeout enabled) ===")
    print(f"{'Messages':<10} | {'Python (sec)':<15} | {'Unilink (sec)':<15} | {'ZeroCopy (sec)':<15}")
    for load in ping_loads:
        t_py = run_python_uds_pingpong(load)
        t_uni = run_unilink_uds_pingpong(load)
        t_zc = run_unilink_uds_pingpong_zerocopy(load)
        
        res_py = f"{t_py:.4f}" if t_py > 0 else "FAIL"
        res_uni = f"{t_uni:.4f}" if t_uni > 0 else "FAIL"
        res_zc = f"{t_zc:.4f}" if t_zc > 0 else "FAIL"
        print(f"{load:<10} | {res_py:<15} | {res_uni:<15} | {res_zc:<15}")

    print("\n=== UDS Throughput (1KB Chunks) ===")
    for load in chunk_loads:
        t_py = run_python_uds_throughput(load)
        t_uni = run_unilink_uds_throughput(load)
        
        res_py = f"{t_py:.4f}" if t_py > 0 else "FAIL"
        res_uni = f"{t_uni:.4f}" if t_uni > 0 else "FAIL"
        print(f"{load:<10} | {res_py:<15} | {res_uni:<15}")

if __name__ == "__main__":
    main()
