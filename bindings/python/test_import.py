import sys
import os

# On Windows with Python 3.8+, DLL dependencies are not automatically loaded from PATH.
# We need to explicitly add the directory containing dependent DLLs (unilink.dll) to the DLL search path.
if os.name == 'nt' and hasattr(os, 'add_dll_directory'):
    for p in sys.path:
        # Check if the path contains the unilink DLL
        dll_path = os.path.join(p, "unilink.dll")
        if os.path.exists(dll_path):
            try:
                os.add_dll_directory(p)
                print(f"Added DLL directory: {p}")
            except Exception as e:
                print(f"Failed to add DLL directory {p}: {e}")

try:
    import unilink_py
    print("Successfully imported unilink_py")
except ImportError as e:
    print(f"Failed to import unilink_py: {e}")
    sys.exit(1)

def test_tcp_client():
    print("Testing TcpClient...")
    client = unilink_py.TcpClient("127.0.0.1", 8080)
    assert not client.is_connected()
    client.auto_manage(True)

    def on_data(data):
        print(f"Data received: {data}")

    client.on_data(on_data)
    print("TcpClient initialized.")

def test_tcp_server():
    print("Testing TcpServer...")
    server = unilink_py.TcpServer(8080)

    def on_connect():
        print("Server connected")

    server.on_connect(on_connect)
    print("TcpServer initialized.")

def test_serial():
    print("Testing Serial...")
    try:
        # Just testing instantiation, not actual hardware
        serial = unilink_py.Serial("/dev/ttyUSB0", 115200)
        serial.set_baud_rate(9600)
        print("Serial initialized.")
    except Exception as e:
        print(f"Serial instantiation failed: {e}")

def test_udp():
    print("Testing Udp...")
    config = unilink_py.UdpConfig()
    config.local_port = 8081
    udp = unilink_py.Udp(config)
    print("Udp initialized.")

if __name__ == "__main__":
    test_tcp_client()
    test_tcp_server()
    test_serial()
    test_udp()
    print("All tests passed!")
