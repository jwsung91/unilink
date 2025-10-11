# Tutorial Examples

Ready-to-compile examples from the unilink tutorials.

---

## 📚 Included Examples

### Tutorial 1: Getting Started

| Example | File | Description | Tutorial Link |
|---------|------|-------------|---------------|
| **Simple Client** | `simple_client.cpp` | Minimal 30-second example | [Tutorial 1](../../docs/tutorials/01_getting_started.md) |
| **My First Client** | `my_first_client.cpp` | Complete first application | [Tutorial 1](../../docs/tutorials/01_getting_started.md) |

### Tutorial 2: TCP Server

| Example | File | Description | Tutorial Link |
|---------|------|-------------|---------------|
| **Echo Server** | `echo_server.cpp` | Basic echo server | [Tutorial 2](../../docs/tutorials/02_tcp_server.md) |
| **Chat Server** | `chat_server.cpp` | Multi-client chat server | [Tutorial 2](../../docs/tutorials/02_tcp_server.md) |

---

## 🚀 Quick Start

### Build All Tutorial Examples

```bash
# From project root
cd build
cmake --build . --target tutorial_simple_client tutorial_my_first_client tutorial_echo_server tutorial_chat_server

# Or build all examples
cmake --build .
```

### Run Examples

#### 1. Simple Client (30 seconds)

**Terminal 1** (Start a test server):
```bash
nc -l 8080
```

**Terminal 2** (Run client):
```bash
./build/examples/tutorials/tutorial_simple_client
```

Type messages in Terminal 1 to send to the client!

---

#### 2. My First Client

**Terminal 1** (Start a test server):
```bash
nc -l 8080
```

**Terminal 2** (Run client with custom host/port):
```bash
# Default: localhost:8080
./build/examples/tutorials/tutorial_my_first_client

# Custom server
./build/examples/tutorials/tutorial_my_first_client 192.168.1.100 9000
```

---

#### 3. Echo Server

**Terminal 1** (Run echo server):
```bash
./build/examples/tutorials/tutorial_echo_server 8080
```

**Terminal 2** (Connect with netcat):
```bash
nc localhost 8080
```

Type messages - they will be echoed back!

**Or use telnet**:
```bash
telnet localhost 8080
```

---

#### 4. Chat Server

**Terminal 1** (Run chat server):
```bash
./build/examples/tutorials/tutorial_chat_server 8080
```

**Terminal 2, 3, 4...** (Connect multiple clients):
```bash
telnet localhost 8080
```

**Chat Commands**:
```
/nick Alice          # Change your nickname
/list                # List all users
/help                # Show help
/quit                # Disconnect
Hello everyone!      # Send a message
```

---

## 📖 Corresponding Tutorials

Each example corresponds to a tutorial in the documentation:

### Tutorial 1: Getting Started
- **Documentation**: [docs/tutorials/01_getting_started.md](../../docs/tutorials/01_getting_started.md)
- **Examples**: 
  - `simple_client.cpp` - The 30-second example
  - `my_first_client.cpp` - Complete walkthrough

**What You'll Learn**:
- Creating your first TCP client
- Connecting to a server
- Sending and receiving data
- Handling connection events

---

### Tutorial 2: Building a TCP Server
- **Documentation**: [docs/tutorials/02_tcp_server.md](../../docs/tutorials/02_tcp_server.md)
- **Examples**:
  - `echo_server.cpp` - Basic echo server
  - `chat_server.cpp` - Complete chat server

**What You'll Learn**:
- Creating a TCP server
- Accepting multiple clients
- Broadcasting messages
- Implementing commands

---

## 🔨 Manual Compilation

If you prefer to compile manually:

```bash
# Simple client
g++ -std=c++17 getting_started/simple_client.cpp \
    -o simple_client \
    -I../../unilink \
    -L../../build \
    -lunilink \
    -lboost_system \
    -pthread

# Echo server
g++ -std=c++17 tcp_server/echo_server.cpp \
    -o echo_server \
    -I../../unilink \
    -L../../build \
    -lunilink \
    -lboost_system \
    -pthread

# Chat server
g++ -std=c++17 tcp_server/chat_server.cpp \
    -o chat_server \
    -I../../unilink \
    -L../../build \
    -lunilink \
    -lboost_system \
    -pthread
```

---

## 🧪 Testing Examples

### Test Simple Client

1. Start test server:
   ```bash
   echo "Hello from server" | nc -l 8080
   ```

2. Run client:
   ```bash
   ./tutorial_simple_client
   ```

3. Expected output:
   ```
   Connected!
   Received: Hello from server
   ```

---

### Test Echo Server

1. Run echo server:
   ```bash
   ./tutorial_echo_server 8080
   ```

2. Connect and test:
   ```bash
   echo "Test message" | nc localhost 8080
   ```

3. Expected output:
   ```
   Welcome to Echo Server!
   Echo: Test message
   ```

---

### Test Chat Server

1. Run chat server:
   ```bash
   ./tutorial_chat_server 8080
   ```

2. Connect multiple clients and chat:
   ```bash
   # Client 1
   telnet localhost 8080
   /nick Alice
   Hello everyone!
   
   # Client 2
   telnet localhost 8080
   /nick Bob
   Hi Alice!
   ```

---

## 💡 Tips

### Debugging

Enable debug logging in the examples:

```cpp
// Add at the beginning of main()
unilink::common::Logger::instance().set_level(unilink::common::LogLevel::DEBUG);
unilink::common::Logger::instance().set_console_output(true);
```

### Testing Without netcat

If you don't have netcat, use the provided echo server:

```bash
# Terminal 1: Run echo server
./tutorial_echo_server 8080

# Terminal 2: Run client
./tutorial_my_first_client 127.0.0.1 8080
```

### Port Already in Use?

If you see "Address already in use":

```bash
# Find what's using the port
lsof -i :8080

# Use a different port
./tutorial_echo_server 8081
./tutorial_my_first_client 127.0.0.1 8081
```

---

## 📚 Next Steps

After trying these examples:

1. **Read the Tutorials**: [docs/tutorials/](../../docs/tutorials/)
2. **Explore API Guide**: [docs/reference/API_GUIDE.md](../../docs/reference/API_GUIDE.md)
3. **Learn Best Practices**: [docs/guides/best_practices.md](../../docs/guides/best_practices.md)
4. **Build Your Own**: Modify these examples or create new ones!

---

## 🆘 Need Help?

- **Can't compile?** See [Troubleshooting Guide](../../docs/guides/troubleshooting.md#compilation-errors)
- **Connection issues?** See [Troubleshooting Guide](../../docs/guides/troubleshooting.md#connection-issues)
- **Questions?** Check the [Documentation Index](../../docs/INDEX.md)

---

**Happy Coding!** 🚀

