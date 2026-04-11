# Python Bindings Guide

Unilink provides high-level Python bindings (`unilink_py`) that mirror the C++ Wrapper API. This allows you to build cross-platform communication applications using Python with the performance of a C++ core.

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

3. **Usage**:
   Add the build output directory (usually `build/lib`) to your `PYTHONPATH`.
   ```bash
   export PYTHONPATH=$PYTHONPATH:$(pwd)/build/lib
   ```

---

## 🔌 TCP Client

The `TcpClient` in Python supports automatic reconnection and asynchronous event handling.

### Basic Usage

```python
import unilink_py
import datetime
import time

# Initialize client
client = unilink_py.TcpClient("127.0.0.1", 8080)

# Configure (Optional)
client.set_retry_interval(datetime.timedelta(milliseconds=2000))
client.set_max_retries(10)
client.set_connection_timeout(datetime.timedelta(milliseconds=5000))

# Register Callbacks
@client.on_connect
def on_connect(ctx):
    print(f"Connected to {ctx.client_info}")

@client.on_data
def on_data(ctx):
    print(f"Received: {ctx.data}")

@client.on_error
def on_error(ctx):
    print(f"Error {ctx.code}: {ctx.message}")

# Start
client.start()

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
import unilink_py

server = unilink_py.TcpServer(8080)

@server.on_client_connect
def on_connect(ctx):
    print(f"Client {ctx.client_id} connected from {ctx.client_info}")

@server.on_data
def on_data(ctx):
    print(f"Message from {ctx.client_id}: {ctx.data}")
    # Echo back
    server.send_to(ctx.client_id, f"Echo: {ctx.data}")

server.start()
print("Server started on port 8080")
```

---

## 📟 Serial Communication

Supports RS-232/422/485 with automatic retry logic.

### Basic Usage

```python
import unilink_py
import datetime

serial = unilink_py.Serial("/dev/ttyUSB0", 115200)

# Configure
serial.set_baud_rate(9600)
serial.set_retry_interval(datetime.timedelta(milliseconds=1000))
serial.auto_manage(True) # Automatically starts

@serial.on_data
def on_data(ctx):
    print(f"Serial Data: {ctx.data}")

# If not using auto_manage, call start()
# serial.start()

serial.send("AT\r\n")
```

---

## 🌐 UDP Communication

Flexible UDP transport with automatic remote endpoint tracking.

### Basic Usage

```python
import unilink_py

config = unilink_py.UdpConfig()
config.local_port = 8081
config.remote_address = "127.0.0.1"
config.remote_port = 8080

udp = unilink_py.Udp(config)

@udp.on_data
def on_data(ctx):
    print(f"UDP Received: {ctx.data}")

@udp.on_error
def on_error(ctx):
    print(f"UDP Error: {ctx.message}")

udp.start()
udp.send("Hello UDP")
```

---

## 📂 UDS Communication

Unix Domain Sockets for high-performance local IPC.

### Basic Usage

```python
import unilink_py

# Initialize client/server with socket path
server = unilink_py.UdsServer("/tmp/myapp.sock")
client = unilink_py.UdsClient("/tmp/myapp.sock")

@server.on_data
def on_server_data(ctx):
    print(f"Server received: {ctx.data}")
    server.send_to(ctx.client_id, "ACK")

@client.on_data
def on_client_data(ctx):
    print(f"Client received: {ctx.data}")

server.start()
client.start()

client.send("Hello over UDS")
```

---

## 🛠️ Advanced Features

### Threading & GIL
The bindings are designed to be thread-safe. When C++ triggers a callback, it automatically acquires the Python GIL (Global Interpreter Lock), allowing you to safely run Python code inside handlers. Long-running operations in Python handlers will block other callbacks for that specific channel, so keep them brief.

### Lifecycle Management
- **`start()`**: Returns a `std::future<bool>` equivalent in Python (usually blocks until initial attempt finishes if wrapped).
- **`stop()`**: Synchronously stops all I/O operations and joins internal threads.
- **`auto_manage(True)`**: When enabled, the channel starts immediately and stops when the Python object is garbage collected.

### Configuration
- **`UdpConfig`**: A dedicated class for UDP settings (buffer sizes, memory pools, etc.).
- **Timeouts**: Methods like `set_retry_interval` accept `datetime.timedelta` objects for intuitive configuration.

```