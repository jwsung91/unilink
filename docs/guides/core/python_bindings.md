# Python Bindings Guide

Unilink provides a Python package (`unilink`) backed by the C++ Wrapper API. This allows you to build cross-platform communication applications using Python with the performance of a C++ core.

---

## 🚀 Getting Started

### Installation

1. **Prerequisites**:
   - Python 3.8+
   - `pybind11` (`pip install pybind11`)
   - CMake and a C++17 compiler

2. **Build**:
   ```bash
   cmake -B build -DBUILD_PYTHON_BINDINGS=ON
   cmake --build build --target unilink_py
   ```

3. **Install**:
   ```bash
   cmake --install build
   ```

4. **Development Usage**:
   If you are running directly from the source and build trees, add both the Python package directory and the build output directory to your `PYTHONPATH`.
   ```bash
   export PYTHONPATH=$(pwd)/bindings/python:$(pwd)/build/lib:$PYTHONPATH
   ```

---

## 🔌 TCP Client

The `TcpClient` in Python supports automatic reconnection and asynchronous event handling.

### Basic Usage

```python
import unilink
import datetime
import time

# Initialize client
client = unilink.TcpClient("127.0.0.1", 8080)

# Configure (Optional)
client.retry_interval(datetime.timedelta(milliseconds=2000))
client.max_retries(10)
client.connection_timeout(datetime.timedelta(milliseconds=5000))

# Register callbacks
def on_connect(ctx):
    print(f"Connected to {ctx.client_info}")

def on_data(ctx):
    print(f"Received: {ctx.data}")

def on_error(ctx):
    print(f"Error {ctx.code}: {ctx.message}")

client.on_connect(on_connect)
client.on_data(on_data)
client.on_error(on_error)

# Start
if not client.start():
    raise RuntimeError("failed to connect")

# Send data
client.send("Hello from Python!")
client.send_line("This is a line.")

# Clean shutdown
time.sleep(1)
client.stop()
```

---

## 🖥️ TCP Server

`TcpServer` manages multiple client connections seamlessly.

### Basic Usage

```python
import unilink

server = unilink.TcpServer(8080)

def on_connect(ctx):
    print(f"Client {ctx.client_id} connected from {ctx.client_info}")

def on_data(ctx):
    print(f"Message from {ctx.client_id}: {ctx.data}")
    # Echo back
    server.send_to(ctx.client_id, f"Echo: {ctx.data}")

server.on_connect(on_connect)
server.on_data(on_data)

if not server.start():
    raise RuntimeError("failed to start server")

print("Server started on port 8080")
```

---

## 📟 Serial Communication

Supports RS-232/422/485 with automatic retry logic.

### Basic Usage

```python
import unilink
import datetime

serial = unilink.Serial("/dev/ttyUSB0", 115200)

# Configure
serial.baud_rate(9600)
serial.retry_interval(datetime.timedelta(milliseconds=1000))
serial.auto_manage(True) # Automatically starts

def on_data(ctx):
    print(f"Serial Data: {ctx.data}")

serial.on_data(on_data)

# If not using auto_manage, call start()
# serial.start()

serial.send("AT\r\n")
```

---

## 🌐 UDP Communication

Flexible UDP transport with automatic remote endpoint tracking.

### Basic Usage

```python
import unilink

config = unilink.UdpConfig()
config.local_port = 8081
config.remote_address = "127.0.0.1"
config.remote_port = 8080

udp = unilink.Udp(config)

def on_data(ctx):
    print(f"UDP Received: {ctx.data}")

def on_error(ctx):
    print(f"UDP Error: {ctx.message}")

udp.on_data(on_data)
udp.on_error(on_error)

if not udp.start():
    raise RuntimeError("failed to start udp transport")

udp.send("Hello UDP")
```

---

## 📂 UDS Communication

Unix Domain Sockets for high-performance local IPC.

### Basic Usage

```python
import unilink

# Initialize client/server with socket path
server = unilink.UdsServer("/tmp/myapp.sock")
client = unilink.UdsClient("/tmp/myapp.sock")

def on_server_data(ctx):
    print(f"Server received: {ctx.data}")
    server.send_to(ctx.client_id, "ACK")

def on_client_data(ctx):
    print(f"Client received: {ctx.data}")

server.on_data(on_server_data)
client.on_data(on_client_data)

if not server.start():
    raise RuntimeError("failed to start uds server")
if not client.start():
    raise RuntimeError("failed to start uds client")

client.send("Hello over UDS")
```

---

## 🛠️ Advanced Features

### Message Framing (Line/Packet)

Unilink allows you to avoid dealing with raw fragmented bytes by using built-in framers. When a framer is set, you use `on_message` instead of `on_data` to receive completely framed payloads.

```python
import unilink

client = unilink.TcpClient("127.0.0.1", 8080)

# Automatically frame incoming bytes by newline ("\n")
client.use_line_framer("\n", include_delimiter=False, max_length=65536)

# Or use a PacketFramer for binary protocols (e.g., [0x02] ... [0x03])
client.use_packet_framer([0x02], [0x03], 1024)

# Client on_message receives a MessageContext, same as on_data.
client.on_message(lambda ctx: print(f"Received complete line: {ctx.data.decode('utf-8')}"))

client.start()
```

For servers, the framer is allocated per-session automatically:

```python
server = unilink.TcpServer(8080)
server.use_line_framer("\n")

# Server's on_message receives a MessageContext with the client_id
server.on_message(lambda ctx: print(f"Client {ctx.client_id} sent: {ctx.data.decode('utf-8')}"))
```

### Threading & GIL
The bindings are designed to be thread-safe. When C++ triggers a callback, it automatically acquires the Python GIL (Global Interpreter Lock), allowing you to safely run Python code inside handlers. Long-running operations in Python handlers will block other callbacks for that specific channel, so keep them brief.

### Lifecycle Management
- **`start()`**: Blocks until the initial start attempt completes and returns `True` or `False`.
- **`stop()`**: Synchronously stops all I/O operations and joins internal threads.
- **`auto_manage(True)`**: When enabled, the channel starts immediately and stops when the Python object is garbage collected.

### Configuration
- **`UdpConfig`**: A dedicated class for UDP settings (buffer sizes, memory pools, etc.).
- **Timeouts**: Methods like `retry_interval` are exposed through pybind11 chrono conversions, so pass `datetime.timedelta` values when those APIs are available on the bound type.
