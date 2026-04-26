import os
import time
import socket
import threading
import statistics
import psutil
import sys

# Configuration levels
LOAD_LEVEL = int(os.getenv("LOAD_LEVEL", "1"))

if LOAD_LEVEL == 1:
    PAYLOAD_SIZE = 1024
    SEND_SLEEP = 0.001
    CHAOS_INTERVAL = 20
    DURATION = 60
elif LOAD_LEVEL == 2:
    PAYLOAD_SIZE = 4096
    SEND_SLEEP = 0.0001
    CHAOS_INTERVAL = 10
    DURATION = 60
elif LOAD_LEVEL == 3:
    PAYLOAD_SIZE = 65536
    SEND_SLEEP = 0.00001 # 10us delay to prevent kernel lockup
    CHAOS_INTERVAL = 7   # Slightly increased from 5
    DURATION = 60
else:
    PAYLOAD_SIZE = 4096
    SEND_SLEEP = 0
    CHAOS_INTERVAL = 2
    DURATION = 180

PORT = 10002
CHAOS_DOWN_TIME = 2

# Match Unilink's reconnect behavior: fast first retry then fixed interval
RETRY_FIRST_MS = 0.1   # 100ms — same as Unilink's first_retry_interval_ms_
RETRY_INTERVAL = 1.0   # 1s   — same as Unilink's DEFAULT_RETRY_INTERVAL_MS

class RawSocketStabilityBench:
    def __init__(self):
        self.server_sock = None
        self.client_sock = None
        self.running = False
        self.server_active = False
        self.sent_bytes = 0
        self.reconnect_count = 0
        self.exceptions = 0
        self.snapshots = []
        self.process = psutil.Process(os.getpid())
        self.server_thread = None
        self.connections = []

    def server_loop(self):
        try:
            self.server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_sock.bind(("127.0.0.1", PORT))
            self.server_sock.listen(128) # Increased backlog
            self.server_sock.settimeout(0.5)
            
            while self.server_active:
                try:
                    conn, addr = self.server_sock.accept()
                    conn.settimeout(0.5)
                    self.connections.append(conn)
                    t = threading.Thread(target=self.handle_client, args=(conn,))
                    t.daemon = True
                    t.start()
                except socket.timeout:
                    continue
                except Exception:
                    break
        finally:
            if self.server_sock:
                self.server_sock.close()
                self.server_sock = None

    def handle_client(self, conn):
        try:
            while self.server_active:
                data = conn.recv(65536)
                if not data: break
        except Exception:
            pass
        finally:
            try: conn.close()
            except: pass
            if conn in self.connections:
                self.connections.remove(conn)

    def start_server(self):
        self.server_active = True
        self.server_thread = threading.Thread(target=self.server_loop)
        self.server_thread.start()

    def stop_server(self):
        self.server_active = False
        for c in self.connections[:]:
            try: c.close()
            except: pass
        if self.server_sock:
            try: self.server_sock.close()
            except: pass
        if self.server_thread:
            self.server_thread.join(timeout=1.0)

    def monitor(self):
        start_time = time.time()
        while self.running and (time.time() - start_time < DURATION):
            prev_sent = self.sent_bytes
            time.sleep(1)
            curr_sent = self.sent_bytes
            throughput = (curr_sent - prev_sent) / (1024 * 1024) # MB/s
            rss = self.process.memory_info().rss / (1024 * 1024) # MB
            self.snapshots.append((throughput, rss))
            
    def sender(self):
        payload = b"A" * PAYLOAD_SIZE
        retry_attempt = 0
        while self.running:
            try:
                if not self.client_sock:
                    try:
                        self.client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                        self.client_sock.settimeout(1.0)
                        self.client_sock.connect(("127.0.0.1", PORT))
                        self.reconnect_count += 1
                        retry_attempt = 0
                    except Exception:
                        self.client_sock = None
                        # Mirror Unilink: fast first retry, then fixed interval
                        sleep_time = RETRY_FIRST_MS if retry_attempt == 0 else RETRY_INTERVAL
                        retry_attempt += 1
                        time.sleep(sleep_time)
                        continue
                
                self.client_sock.sendall(payload)
                self.sent_bytes += PAYLOAD_SIZE
                if SEND_SLEEP > 0:
                    time.sleep(SEND_SLEEP)
            except (socket.error, BrokenPipeError, ConnectionResetError):
                self.exceptions += 1
                if self.client_sock:
                    try: self.client_sock.close()
                    except: pass
                self.client_sock = None
                retry_attempt = 0
            except Exception as e:
                self.exceptions += 1
                self.client_sock = None
                retry_attempt = 0

    def chaos(self):
        start_time = time.time()
        while self.running and (time.time() - start_time < DURATION):
            time.sleep(CHAOS_INTERVAL)
            if not self.running: break
            self.stop_server()
            time.sleep(CHAOS_DOWN_TIME)
            if not self.running: break
            self.start_server()

    def run(self):
        print(f"--- Starting Raw Socket Stability Bench (Level {LOAD_LEVEL}) ---")
        print(f"Payload: {PAYLOAD_SIZE} bytes, Delay: {SEND_SLEEP}s, Chaos: {CHAOS_INTERVAL}s")
        
        self.running = True
        self.start_server()
        
        threads = [
            threading.Thread(target=self.sender, name="Sender"),
            threading.Thread(target=self.chaos, name="Chaos"),
            threading.Thread(target=self.monitor, name="Monitor")
        ]

        for t in threads: t.start()
        
        try:
            start_run = time.time()
            while self.running and (time.time() - start_run < DURATION):
                time.sleep(10); print(f"[Heartbeat] {int(time.time() - start_run)}s elapsed...")
        except KeyboardInterrupt:
            print("\nBenchmark interrupted by user")
        finally:
            self.running = False
            for t in threads: t.join(timeout=2.0)
            self.stop_server()
            if self.client_sock: 
                try: self.client_sock.close()
                except: pass
        
        self.report()

    def report(self):
        all_throughputs = [s[0] for s in self.snapshots]
        rss_values = [s[1] for s in self.snapshots]
        
        avg_tp = statistics.mean(all_throughputs) if all_throughputs else 0
        std_tp = statistics.stdev(all_throughputs) if len(all_throughputs) > 1 else 0
        mem_drift = rss_values[-1] - rss_values[0] if rss_values else 0
        peak_rss = max(rss_values) if rss_values else 0
        
        print(f"\n--- Raw Socket Stability Results (Level {LOAD_LEVEL}) ---")
        print(f"Reconnect Successes: {self.reconnect_count}")
        print(f"Exceptions:          {self.exceptions}")
        print(f"Peak Memory (RSS):   {peak_rss:.2f} MB")
        print(f"Memory Drift:        {mem_drift:.2f} MB")
        print(f"Throughput Avg:      {avg_tp:.2f} MB/s")
        print(f"Throughput StdDev:   {std_tp:.2f} MB/s")
        print(f"----------------------------------------------------")

if __name__ == "__main__":
    bench = RawSocketStabilityBench()
    bench.run()
