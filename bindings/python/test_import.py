import sys
import os

# On Windows with Python 3.8+, DLL dependencies are not automatically loaded from PATH.
# We need to explicitly add the directory containing dependent DLLs (unilink.dll) to the DLL search path.
if os.name == 'nt' and hasattr(os, 'add_dll_directory'):
    found_dll = False

    # 1. Search in sys.path
    for p in sys.path:
        # Check if the path contains the unilink DLL
        dll_path = os.path.join(p, "unilink.dll")
        if os.path.exists(dll_path):
            try:
                os.add_dll_directory(p)
                print(f"Added DLL directory from sys.path: {p}")
                found_dll = True
            except Exception as e:
                print(f"Failed to add DLL directory {p}: {e}")

    # 2. Search relative to this script
    script_dir = os.path.dirname(os.path.abspath(__file__))
    candidates = [
        script_dir,
        os.path.join(script_dir, "..", "..", "bin"),
        os.path.join(script_dir, "..", "..", "bin", "Release"),
        os.path.join(script_dir, "..", "..", "bin", "Debug"),
        os.path.join(script_dir, "..", "..", "bin", "RelWithDebInfo"),
        os.path.join(script_dir, "..", "..", "lib"),
        os.path.join(script_dir, "..", "..", "lib", "Release"),
        os.path.join(script_dir, "..", "..", "lib", "Debug"),
        os.path.join(script_dir, "..", "..", "lib", "RelWithDebInfo"),
    ]

    for p in candidates:
        abs_p = os.path.abspath(p)
        if os.path.exists(os.path.join(abs_p, "unilink.dll")):
            try:
                os.add_dll_directory(abs_p)
                print(f"Added DLL directory from candidates: {abs_p}")
                found_dll = True
            except Exception as e:
                print(f"Failed to add DLL directory {abs_p}: {e}")

    if not found_dll:
        print("WARNING: unilink.dll not found in standard search paths. Import may fail.")

try:
    import unilink_py
    print("Successfully imported unilink_py")
except ImportError as e:
    print(f"Failed to import unilink_py: {e}")
    sys.exit(1)

def test_tcp_client():
    print("Testing TcpClient...")
    import datetime
    client = unilink_py.TcpClient("127.0.0.1", 8080)
    assert not client.connected()
    client.auto_manage(True)
    client.retry_interval(datetime.timedelta(milliseconds=1000))
    client.max_retries(5)
    client.connection_timeout(datetime.timedelta(milliseconds=5000))

    def on_data(ctx):
        print(f"Data received from client {ctx.client_id}: {ctx.data}")

    client.on_data(on_data)
    print("TcpClient initialized.")

def test_tcp_server():
    print("Testing TcpServer...")
    server = unilink_py.TcpServer(8080)

    def on_connect(ctx):
        print(f"Client {ctx.client_id} connected to server")

    server.on_connect(on_connect)
    print("TcpServer initialized.")

def test_serial():
    print("Testing Serial...")
    import datetime
    try:
        # Just testing instantiation, not actual hardware
        serial = unilink_py.Serial("/dev/ttyUSB0", 115200)
        serial.baud_rate(9600)
        serial.auto_manage(True)
        serial.retry_interval(datetime.timedelta(milliseconds=2000))
        print("Serial initialized.")
    except Exception as e:
        print(f"Serial instantiation failed (expected if no dev): {e}")

def test_udp():
    print("Testing Udp...")
    config = unilink_py.UdpConfig()
    config.local_port = 8081
    udp = unilink_py.Udp(config)
    udp.auto_manage(True)

    def on_connect(ctx):
        print(f"Udp connected: {ctx.client_id}")

    def on_disconnect(ctx):
        print(f"Udp disconnected: {ctx.client_id}")

    def on_error(ctx):
        print(f"Udp error: {ctx.code}")

    udp.on_connect(on_connect)
    udp.on_disconnect(on_disconnect)
    udp.on_error(on_error)

    print("Udp initialized.")

if __name__ == "__main__":
    test_tcp_client()
    test_tcp_server()
    test_serial()
    test_udp()
    print("All tests passed!")
