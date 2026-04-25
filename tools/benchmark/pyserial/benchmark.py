import subprocess
import time
import threading
import serial
import unilink
import sys
import contextlib

PING_MESSAGE = b"PING"
PING_LEN = len(PING_MESSAGE)

CHUNK_SIZE = 1024
CHUNK_MESSAGE = b"A" * CHUNK_SIZE

@contextlib.contextmanager
def get_virtual_ports():
    socat_proc = subprocess.Popen(
        ['socat', '-d', '-d', 'pty,raw,echo=0', 'pty,raw,echo=0'],
        stderr=subprocess.PIPE,
        universal_newlines=True
    )
    master_port = None
    slave_port = None
    for line in socat_proc.stderr:
        if "PTY is " in line:
            port = line.split("PTY is ")[1].strip()
            if master_port is None:
                master_port = port
            else:
                slave_port = port
                break
    
    # Allow time for ports to be fully ready
    time.sleep(0.2)
    
    # Drain stderr to prevent socat from blocking on full pipe buffer
    def drain_stderr():
        for _ in socat_proc.stderr:
            pass
            
    drain_thread = threading.Thread(target=drain_stderr, daemon=True)
    drain_thread.start()
    
    try:
        yield master_port, slave_port
    finally:
        socat_proc.terminate()
        try:
            socat_proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            socat_proc.kill()
            socat_proc.wait()

def run_pyserial_pingpong(master_port, slave_port, num_pings):
    master = serial.Serial(master_port, 115200, timeout=2, write_timeout=2)
    slave = serial.Serial(slave_port, 115200, timeout=2, write_timeout=2)
    time.sleep(0.1)

    done = threading.Event()
    error_msg = []

    def slave_echo():
        try:
            for _ in range(num_pings):
                data = slave.read(PING_LEN)
                if not data or len(data) != PING_LEN:
                    error_msg.append(f"Slave read incomplete")
                    break
                slave.write(data)
        except Exception as e:
            error_msg.append(f"Slave exception: {e}")
        finally:
            done.set()

    t = threading.Thread(target=slave_echo)
    t.start()

    start_time = time.time()
    try:
        for _ in range(num_pings):
            master.write(PING_MESSAGE)
            echo = master.read(PING_LEN)
            if len(echo) != PING_LEN:
                error_msg.append(f"Master read incomplete")
                break
    except Exception as e:
        error_msg.append(f"Master exception: {e}")
            
    end_time = time.time()
    
    timeout_sec = max(5.0, num_pings * 0.02)
    done.wait(timeout=timeout_sec)
    
    master.close()
    slave.close()
    t.join(timeout=1)
    
    if error_msg:
        return -1, error_msg[0]
    return end_time - start_time, None

def run_unilink_pingpong(master_port, slave_port, num_pings):
    master = unilink.Serial(master_port, 115200)
    slave = unilink.Serial(slave_port, 115200)

    completed_pings = 0
    master_buffer = bytearray()
    done = threading.Event()

    def on_slave_data(ctx):
        slave.send(ctx.data)

    def on_master_data(ctx):
        nonlocal completed_pings, master_buffer
        master_buffer.extend(ctx.data)
        
        while len(master_buffer) >= PING_LEN:
            del master_buffer[:PING_LEN]
            completed_pings += 1
            if completed_pings < num_pings:
                master.send(PING_MESSAGE)
            else:
                done.set()

    slave.on_data(on_slave_data)
    master.on_data(on_master_data)
    
    master.start()
    slave.start()
    time.sleep(0.2)

    start_time = time.time()
    master.send(PING_MESSAGE)
    
    timeout_sec = max(5.0, num_pings * 0.02)
    success = done.wait(timeout=timeout_sec)
    end_time = time.time()
    
    master.stop()
    slave.stop()
    
    if not success:
        return -1, f"Timeout: {completed_pings}/{num_pings} pings"
    return end_time - start_time, None

def run_pyserial_throughput(master_port, slave_port, num_chunks):
    master = serial.Serial(master_port, 115200, timeout=2, write_timeout=2)
    slave = serial.Serial(slave_port, 115200, timeout=2, write_timeout=2)
    time.sleep(0.1)

    total_bytes = CHUNK_SIZE * num_chunks
    received_bytes = 0
    done = threading.Event()
    error_msg = []

    def slave_read():
        nonlocal received_bytes
        try:
            while received_bytes < total_bytes:
                data = slave.read(min(4096, total_bytes - received_bytes))
                if not data:
                    error_msg.append(f"Slave timeout")
                    break
                received_bytes += len(data)
        except Exception as e:
            error_msg.append(f"Slave exception: {e}")
        finally:
            done.set()

    t = threading.Thread(target=slave_read)
    t.start()

    start_time = time.time()
    try:
        for _ in range(num_chunks):
            master.write(CHUNK_MESSAGE)
            time.sleep(0.001)
    except Exception as e:
        error_msg.append(f"Master exception: {e}")
            
    timeout_sec = max(5.0, num_chunks * 0.05)
    done.wait(timeout=timeout_sec)
    end_time = time.time()
    
    master.close()
    slave.close()
    t.join(timeout=1)
    
    if error_msg:
        return -1, error_msg[0]
    if received_bytes != total_bytes:
        return -1, f"Mismatch: {received_bytes} != {total_bytes}"
    return end_time - start_time, None

def run_unilink_throughput(master_port, slave_port, num_chunks):
    master = unilink.Serial(master_port, 115200)
    slave = unilink.Serial(slave_port, 115200)

    total_bytes = CHUNK_SIZE * num_chunks
    received_bytes = 0
    done = threading.Event()

    def on_slave_data(ctx):
        nonlocal received_bytes
        received_bytes += len(ctx.data)
        if received_bytes >= total_bytes:
            done.set()

    slave.on_data(on_slave_data)
    
    master.start()
    slave.start()
    time.sleep(0.2)

    start_time = time.time()
    for _ in range(num_chunks):
        master.send(CHUNK_MESSAGE)
        time.sleep(0.001)
    
    timeout_sec = max(5.0, num_chunks * 0.05)
    success = done.wait(timeout=timeout_sec)
    end_time = time.time()
    
    master.stop()
    slave.stop()
    
    if not success:
        return -1, f"Timeout: {received_bytes}/{total_bytes}"
    return end_time - start_time, None

def main():
    ping_loads = [10, 50, 100, 500, 1000]
    chunk_loads = [10, 50, 100, 500]

    print("============================================================")
    print("--- 1. Progressive Latency Test (Ping-Pong, 4 bytes/msg) ---")
    print(f"{'Messages':<10} | {'pyserial (sec)':<20} | {'unilink (sec)':<20}")
    print("-" * 60)
    for load in ping_loads:
        with get_virtual_ports() as (m_port, s_port):
            t_py, err_py = run_pyserial_pingpong(m_port, s_port, load)
            res_py = f"{t_py:.4f}" if t_py > 0 else f"FAIL ({err_py})"
            
        time.sleep(0.2)
        
        with get_virtual_ports() as (m_port, s_port):
            t_uni, err_uni = run_unilink_pingpong(m_port, s_port, load)
            res_uni = f"{t_uni:.4f}" if t_uni > 0 else f"FAIL ({err_uni})"
            
        print(f"{load:<10} | {res_py:<20} | {res_uni:<20}")

    print("\n--- 2. Progressive Throughput Test (Streaming, 1KB/chunk) ---")
    print(f"{'Chunks':<10} | {'pyserial (sec)':<20} | {'unilink (sec)':<20}")
    print("-" * 60)
    for load in chunk_loads:
        with get_virtual_ports() as (m_port, s_port):
            t_py, err_py = run_pyserial_throughput(m_port, s_port, load)
            res_py = f"{t_py:.4f}" if t_py > 0 else f"FAIL ({err_py})"
            
        time.sleep(0.2)
        
        with get_virtual_ports() as (m_port, s_port):
            t_uni, err_uni = run_unilink_throughput(m_port, s_port, load)
            res_uni = f"{t_uni:.4f}" if t_uni > 0 else f"FAIL ({err_uni})"
            
        print(f"{load:<10} | {res_py:<20} | {res_uni:<20}")

if __name__ == "__main__":
    main()