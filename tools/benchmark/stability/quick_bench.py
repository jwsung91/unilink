import unilink_py as unilink
import time
import threading

def run_unilink(duration_s):
    server = unilink.TcpServer(10012)
    recv_bytes = [0]
    def on_data(ctx):
        recv_bytes[0] += len(ctx.data)
    server.on_data(on_data)
    server.start_sync()

    client = unilink.TcpClient("127.0.0.1", 10012)
    client.backpressure_strategy = unilink.BackpressureStrategy.Reliable
    client.start_sync()

    running = True
    sent_bytes = 0
    payload = b"A" * 65536

    start_time = time.time()
    while time.time() - start_time < duration_s:
        if client.send(payload):
            sent_bytes += len(payload)
        else:
            time.sleep(0.00001)

    running = False
    time.sleep(0.5)
    server.stop()
    client.stop()

    elapsed = time.time() - start_time
    print(f"--- Unilink Python ---")
    print(f"Sent: {sent_bytes / (1024*1024):.2f} MB")
    print(f"Recv: {recv_bytes[0] / (1024*1024):.2f} MB")
    print(f"Throughput: {(sent_bytes / (1024*1024)) / elapsed:.2f} MB/s")

run_unilink(10)
