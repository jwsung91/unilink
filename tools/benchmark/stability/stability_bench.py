import time
import threading
import unilink
import os
import sys

# Constants
TEST_DURATION = 60  # Duration in seconds for a quick stability run
PAYLOAD_SIZE = 4096 # 4KB chunks
HOST = "127.0.0.1"
PORT = 9990

class StabilityMonitor:
    def __init__(self):
        self.start_memory = self.get_memory_usage()
        self.max_memory = self.start_memory
        self.reconnect_count = 0
        self.error_count = 0
        self.total_bytes = 0
        self.is_running = True

    def get_memory_usage(self):
        # Returns RSS memory in MB
        try:
            import psutil
            process = psutil.Process(os.getpid())
            return process.memory_info().rss / 1024 / 1024
        except ImportError:
            # Fallback for Linux if psutil is not available
            try:
                with open('/proc/self/status', 'r') as f:
                    for line in f:
                        if line.startswith('VmRSS:'):
                            return int(line.split()[1]) / 1024
            except:
                return 0
        return 0

    def monitor_loop(self):
        while self.is_running:
            current_mem = self.get_memory_usage()
            self.max_memory = max(self.max_memory, current_mem)
            time.sleep(1)

    def print_report(self, duration):
        end_memory = self.get_memory_usage()
        print("\n" + "="*50)
        print("🛡️ UNILINK STABILITY REPORT")
        print("="*50)
        print(f"Test Duration:      {duration:.1f} seconds")
        print(f"Total Data Xfer:    {self.total_bytes / 1024 / 1024:.2f} MB")
        print(f"Average Throughput: {self.total_bytes / 1024 / 1024 / duration:.2f} MB/s")
        print("-"*50)
        print(f"Start Memory (RSS): {self.start_memory:.2f} MB")
        print(f"Peak Memory (RSS):  {self.max_memory:.2f} MB")
        print(f"End Memory (RSS):   {end_memory:.2f} MB")
        print(f"Memory Drift:       {end_memory - self.start_memory:+.4f} MB")
        print("-"*50)
        print(f"Successful Reconnects: {self.reconnect_count}")
        print(f"Total Errors:          {self.error_count}")
        
        if (end_memory - self.start_memory) < 1.0 and self.error_count == 0:
            print("\n✅ STATUS: ROCK SOLID (Perfect memory flatness & zero errors)")
        elif self.error_count > 0:
            print("\n⚠️ STATUS: UNSTABLE (Errors detected)")
        else:
            print("\nℹ️ STATUS: OBSERVATION NEEDED (Minor memory drift)")
        print("="*50)

def run_stability_test():
    monitor = StabilityMonitor()
    monitor_thread = threading.Thread(target=monitor.monitor_loop)
    monitor_thread.start()

    server = unilink.TcpServer(PORT)
    client = unilink.TcpClient(HOST, PORT)

    # Use Batching to stress GIL and memory pool
    def on_server_batch(batch):
        monitor.total_bytes += sum(len(ctx.data) for ctx in batch)

    def on_client_error(ctx):
        monitor.error_count += 1
        # print(f"Client error: {ctx.message}")

    def on_client_connect(ctx):
        monitor.reconnect_count += 1

    server.on_data_batch(on_server_batch)
    client.on_error(on_client_error)
    client.on_connect(on_client_connect)

    server.start()
    client.start()
    
    # We expect one initial connect
    time.sleep(1)
    monitor.reconnect_count = 0 

    start_time = time.time()
    payload = b"S" * PAYLOAD_SIZE

    print(f"🚀 Starting high-load stability test for {TEST_DURATION}s...")
    
    # Chaos Simulation: periodically stop and start the server
    def chaos_loop():
        while time.time() - start_time < TEST_DURATION - 5:
            time.sleep(10)
            # print("🔥 Chaos: Restarting Server...")
            server.stop()
            time.sleep(2)
            server.start()

    chaos_thread = threading.Thread(target=chaos_loop, daemon=True)
    chaos_thread.start()

    # Data Flood Loop
    while time.time() - start_time < TEST_DURATION:
        try:
            if client.connected():
                if not client.send(payload):
                    # Backpressure hit
                    time.sleep(0.01)
            else:
                time.sleep(0.1)
        except:
            monitor.error_count += 1

    actual_duration = time.time() - start_time
    monitor.is_running = False
    monitor_thread.join()
    
    client.stop()
    server.stop()
    
    monitor.print_report(actual_duration)

if __name__ == "__main__":
    run_stability_test()
