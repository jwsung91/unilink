import os
import time
import threading
import statistics
import unilink
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
    SEND_SLEEP = 0.00001  # 10μs - prevent strand queue starvation under max load
    CHAOS_INTERVAL = 7
    DURATION = 60
else:
    PAYLOAD_SIZE = 4096
    SEND_SLEEP = 0.00001  # minimum throttle even for ultra stress
    CHAOS_INTERVAL = 2
    DURATION = 180

PORT = 10001
CHAOS_DOWN_TIME = 2

class UnilinkStabilityBench:
    def __init__(self):
        self.server = None
        self.client = None
        self.running = False
        self.sent_bytes = 0
        self.reconnect_count = 0
        self.exceptions = 0
        self.bp_events_triggered = 0
        self.snapshots = []
        self.process = psutil.Process(os.getpid())
        self.send_allowed = threading.Event()
        self.send_allowed.set()

    def start_server(self):
        try:
            self.server = unilink.TcpServer(PORT)
            self.server.on_data(lambda ctx: None) # Sink
            self.server.start_sync()
        except Exception as e:
            self.exceptions += 1

    def stop_server(self):
        if self.server:
            try:
                self.server.stop()
                self.server = None
            except Exception:
                pass

    def on_bp(self, queued_bytes):
        # C++ fires ON  when queued_bytes >= bp_high_ (default 16 MiB)
        # C++ fires OFF when queued_bytes <= bp_low_  (default  8 MiB = bp_high_/2)
        # Distinguish ON from OFF by comparing against the midpoint (bp_low_).
        if queued_bytes > 8 * 1024 * 1024:  # ON event: value >= 16 MiB
            self.send_allowed.clear()
            self.bp_events_triggered += 1
        else:                                # OFF event: value <= 8 MiB
            self.send_allowed.set()

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
        while self.running:
            try:
                # Flow control
                self.send_allowed.wait(timeout=0.1)
                if not self.send_allowed.is_set():
                    continue

                if self.client and self.client.connected():
                    if self.client.send(payload):
                        self.sent_bytes += PAYLOAD_SIZE
                    else:
                        # Send failed (backpressure hard limit or disconnected)
                        time.sleep(0.001) 
                    
                    if SEND_SLEEP > 0:
                        time.sleep(SEND_SLEEP)
                else:
                    self.send_allowed.set() # Reset if disconnected
                    time.sleep(0.01) 
            except Exception:
                self.exceptions += 1

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
        print(f"--- Starting Unilink Stability Bench (Level {LOAD_LEVEL}) ---")
        print(f"Payload: {PAYLOAD_SIZE} bytes, Flow Control: ON, Chaos: {CHAOS_INTERVAL}s")
        
        self.running = True
        self.start_server()
        
        self.client = unilink.TcpClient("127.0.0.1", PORT)
        self.client.on_connect(lambda ctx: setattr(self, 'reconnect_count', self.reconnect_count + 1))
        self.client.on_backpressure(self.on_bp)
        
        # Optimize
        self.client.batch_size(200)
        from datetime import timedelta
        self.client.batch_latency(timedelta(milliseconds=1))
        
        try:
            self.client.start_sync()
        except Exception:
            self.running = False
            return

        threads = [
            threading.Thread(target=self.sender, name="Sender"),
            threading.Thread(target=self.chaos, name="Chaos"),
            threading.Thread(target=self.monitor, name="Monitor")
        ]

        for t in threads: t.start()
        
        try:
            start_run = time.time()
            while self.running and (time.time() - start_run < DURATION):
                time.sleep(10)
                print(f"[Heartbeat] {int(time.time() - start_run)}s elapsed... BP count: {self.bp_events_triggered}")
        except KeyboardInterrupt:
            print("\nBenchmark interrupted")
        finally:
            self.running = False
            self.send_allowed.set() # Release sender
            for t in threads: t.join(timeout=2.0)
            self.stop_server()
            if self.client: self.client.stop()
        
        self.report()

    def report(self):
        all_throughputs = [s[0] for s in self.snapshots]
        rss_values = [s[1] for s in self.snapshots]
        
        avg_tp = statistics.mean(all_throughputs) if all_throughputs else 0
        std_tp = statistics.stdev(all_throughputs) if len(all_throughputs) > 1 else 0
        mem_drift = rss_values[-1] - rss_values[0] if rss_values else 0
        peak_rss = max(rss_values) if rss_values else 0
        
        print(f"\n--- Unilink Stability Results (Level {LOAD_LEVEL}) ---")
        print(f"Reconnect Successes: {self.reconnect_count}")
        print(f"Exceptions:          {self.exceptions}")
        print(f"Backpressure Hits:   {self.bp_events_triggered}")
        print(f"Peak Memory (RSS):   {peak_rss:.2f} MB")
        print(f"Memory Drift:        {mem_drift:.2f} MB")
        print(f"Throughput Avg:      {avg_tp:.2f} MB/s")
        print(f"Throughput StdDev:   {std_tp:.2f} MB/s")
        print(f"--------------------------------------------------")

if __name__ == "__main__":
    bench = UnilinkStabilityBench()
    bench.run()
