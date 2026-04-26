
import time
import sys
import os

# Add python bindings to path
sys.path.append(os.path.join(os.getcwd(), "build/bindings/python"))
import unilink_py as unilink

def run_bench(strategy_name, threshold_mb, strategy_enum):
    print(f"\n>>> Running Benchmark: {strategy_name} ({threshold_mb}MB Threshold)")
    
    # Using simple constructor and property setters
    server = unilink.TcpServer(10001)
    server.backpressure_threshold = int(threshold_mb * 1024 * 1024)
    server.backpressure_strategy = strategy_enum
    
    received_count = 0
    latencies = []
    
    def on_msg(ctx):
        nonlocal received_count
        received_count += 1
        # Extract timestamp from message (first part before '|')
        try:
            data_str = ctx.data.decode()
            sent_at = float(data_str.split('|')[0])
            latencies.append(time.time() - sent_at)
        except Exception as e:
            pass

    server.on_message(on_msg)
    server.start()
    
    # Client setup
    client = unilink.TcpClient("127.0.0.1", 10001)
    client.backpressure_threshold = int(threshold_mb * 1024 * 1024)
    client.backpressure_strategy = strategy_enum
    
    client.start()
    time.sleep(1) # Wait for connection
    
    # Simulation: Heavy load (High FPS)
    duration = 2
    start_time = time.time()
    sent_count = 0
    
    payload = "X" * 1024 * 100 # 100KB per frame
    
    print(f"Simulating heavy load for {duration} seconds...")
    while time.time() - start_time < duration:
        # Every 5ms send a frame (~20MB/s)
        msg = f"{time.time()}|{payload}"
        client.send(msg)
        sent_count += 1
        time.sleep(0.005) 
        
    print("Cooling down...")
    time.sleep(1) # Wait for remaining messages
    
    server.stop()
    client.stop()
    
    if latencies:
        avg_lat = sum(latencies) / len(latencies) * 1000
        max_lat = max(latencies) * 1000
        p99_lat = sorted(latencies)[int(len(latencies)*0.99)] * 1000
        print(f"Results for {strategy_name}:")
        print(f"  Sent: {sent_count}, Received: {received_count}")
        print(f"  Average Latency: {avg_lat:.2f} ms")
        print(f"  P99 Latency: {p99_lat:.2f} ms")
        print(f"  Max Latency: {max_lat:.2f} ms")
        return avg_lat, p99_lat
    else:
        print("No data received. Check if connection was successful.")
        return None, None

if __name__ == "__main__":
    print("Unilink Adaptive Strategy (Freshness) Benchmark")
    print("==============================================")
    
    # 1. Wait Strategy (Standard) - Large buffer causes bloat
    run_bench("Wait (Standard)", 16, unilink.BackpressureStrategy.Wait)
    
    # 2. Latest Strategy (Adaptive) - Small buffer + Drop keeps it fresh
    run_bench("Latest (Adaptive)", 0.5, unilink.BackpressureStrategy.Latest)
