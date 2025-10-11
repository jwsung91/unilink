# Tutorial 2: Building a TCP Server

Learn how to create a robust TCP server with unilink.

**Duration**: 20 minutes  
**Difficulty**: Beginner to Intermediate  
**Prerequisites**: [Tutorial 1: Getting Started](01_getting_started.md)

---

## What You'll Build

A TCP echo server that:
1. Accepts multiple client connections
2. Echoes back any received data
3. Tracks connected clients
4. Handles client disconnections gracefully

---

## Step 1: Basic Server Setup

Create `echo_server.cpp`:

```cpp
#include <iostream>
#include <atomic>
#include <thread>
#include "unilink/unilink.hpp"

class EchoServer {
private:
    std::shared_ptr<unilink::wrapper::TcpServer> server_;
    std::atomic<int> client_count_{0};
    uint16_t port_;

public:
    EchoServer(uint16_t port) : port_(port) {}
    
    void start() {
        std::cout << "Starting echo server on port " << port_ << std::endl;
        
        server_ = unilink::tcp_server(port_)
            .on_connect([this](size_t client_id, const std::string& ip) {
                handle_connect(client_id, ip);
            })
            .on_data([this](size_t client_id, const std::string& data) {
                handle_data(client_id, data);
            })
            .on_disconnect([this](size_t client_id) {
                handle_disconnect(client_id);
            })
            .on_error([this](const std::string& error) {
                handle_error(error);
            })
            .build();
        
        server_->start();
        std::cout << "Server started! Waiting for connections..." << std::endl;
    }
    
    void handle_connect(size_t client_id, const std::string& ip) {
        client_count_++;
        std::cout << "[Client " << client_id << "] Connected from " << ip 
                  << " (Total clients: " << client_count_ << ")" << std::endl;
        
        // Send welcome message
        server_->send_to_client(client_id, "Welcome to Echo Server!\n");
    }
    
    void handle_data(size_t client_id, const std::string& data) {
        std::cout << "[Client " << client_id << "] Received: " << data << std::endl;
        
        // Echo back the data
        server_->send_to_client(client_id, "Echo: " + data);
    }
    
    void handle_disconnect(size_t client_id) {
        client_count_--;
        std::cout << "[Client " << client_id << "] Disconnected "
                  << "(Remaining clients: " << client_count_ << ")" << std::endl;
    }
    
    void handle_error(const std::string& error) {
        std::cerr << "[Error] " << error << std::endl;
    }
    
    void stop() {
        if (server_) {
            std::cout << "Stopping server..." << std::endl;
            server_->send("Server shutting down. Goodbye!\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            server_->stop();
        }
    }
    
    bool is_running() const {
        return server_ && server_->is_listening();
    }
};

int main(int argc, char** argv) {
    uint16_t port = (argc > 1) ? std::stoi(argv[1]) : 8080;
    
    EchoServer server(port);
    server.start();
    
    // Run until user presses Ctrl+C
    std::cout << "Press Ctrl+C to stop the server" << std::endl;
    while (server.is_running()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    server.stop();
    return 0;
}
```

---

## Step 2: Build and Test

### Compile
```bash
g++ -std=c++17 echo_server.cpp -o echo_server -lunilink -lboost_system -pthread
```

### Run Server
```bash
./echo_server 8080
```

### Test with Multiple Clients

**Terminal 1** (Server):
```bash
./echo_server 8080
```

**Terminal 2** (Client 1):
```bash
telnet localhost 8080
```

**Terminal 3** (Client 2):
```bash
telnet localhost 8080
```

Type messages in each client terminal and see them echoed back!

---

## Step 3: Add Client Management

Enhanced version with client tracking:

```cpp
#include <map>
#include <mutex>

class AdvancedEchoServer {
private:
    struct ClientInfo {
        std::string ip;
        std::chrono::system_clock::time_point connected_at;
        size_t messages_sent{0};
    };
    
    std::shared_ptr<unilink::wrapper::TcpServer> server_;
    std::map<size_t, ClientInfo> clients_;
    std::mutex clients_mutex_;
    uint16_t port_;

public:
    AdvancedEchoServer(uint16_t port) : port_(port) {}
    
    void start() {
        server_ = unilink::tcp_server(port_)
            .on_connect([this](size_t id, const std::string& ip) {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                clients_[id] = {ip, std::chrono::system_clock::now(), 0};
                
                std::cout << "[Client " << id << "] Connected from " << ip << std::endl;
                server_->send_to_client(id, "Welcome! You are client #" + 
                                       std::to_string(id) + "\n");
            })
            .on_data([this](size_t id, const std::string& data) {
                {
                    std::lock_guard<std::mutex> lock(clients_mutex_);
                    if (clients_.count(id)) {
                        clients_[id].messages_sent++;
                    }
                }
                
                // Handle commands
                if (data == "/stats\n") {
                    send_statistics(id);
                } else if (data == "/clients\n") {
                    send_client_list(id);
                } else {
                    // Echo normally
                    server_->send_to_client(id, "Echo: " + data);
                }
            })
            .on_disconnect([this](size_t id) {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                clients_.erase(id);
                std::cout << "[Client " << id << "] Disconnected" << std::endl;
            })
            .build();
        
        server_->start();
        std::cout << "Advanced Echo Server started on port " << port_ << std::endl;
    }
    
    void send_statistics(size_t client_id) {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        
        if (clients_.count(client_id) == 0) return;
        
        auto& info = clients_[client_id];
        auto duration = std::chrono::system_clock::now() - info.connected_at;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        
        std::string stats = "=== Your Statistics ===\n";
        stats += "IP: " + info.ip + "\n";
        stats += "Connected for: " + std::to_string(seconds) + " seconds\n";
        stats += "Messages sent: " + std::to_string(info.messages_sent) + "\n";
        stats += "======================\n";
        
        server_->send_to_client(client_id, stats);
    }
    
    void send_client_list(size_t client_id) {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        
        std::string list = "=== Connected Clients ===\n";
        for (const auto& [id, info] : clients_) {
            list += "Client " + std::to_string(id) + ": " + info.ip;
            if (id == client_id) list += " (you)";
            list += "\n";
        }
        list += "Total: " + std::to_string(clients_.size()) + " clients\n";
        list += "========================\n";
        
        server_->send_to_client(client_id, list);
    }
    
    void stop() {
        if (server_) {
            server_->send("Server shutting down...\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            server_->stop();
        }
    }
};
```

---

## Step 4: Single Client Mode

For applications that only need one client at a time:

```cpp
auto server = unilink::tcp_server(8080)
    .single_client()  // Only accept one client
    .on_connect([](size_t id, const std::string& ip) {
        std::cout << "Client connected: " << ip << std::endl;
        // Previous client is automatically disconnected
    })
    .on_data([](size_t id, const std::string& data) {
        std::cout << "Received: " << data << std::endl;
    })
    .build();

server->start();
// ... do work ...
server->stop();  // Clean shutdown when done
```

---

## Step 5: Port Retry Logic

Handle cases where the port might be in use:

```cpp
auto server = unilink::tcp_server(8080)
    .enable_port_retry(
        true,   // Enable retry
        5,      // Try 5 times
        1000    // Wait 1 second between attempts
    )
    .on_error([](const std::string& error) {
        std::cerr << "Server error: " << error << std::endl;
    })
    .build();

server->start();

// Check if server started successfully
std::this_thread::sleep_for(std::chrono::seconds(6)); // Wait for retries
if (!server->is_listening()) {
    std::cerr << "Failed to start server after retries" << std::endl;
    return 1;
}

// ... do work ...
server->stop();  // Clean shutdown
```

---

## Step 6: Broadcasting to All Clients

Send messages to all connected clients:

```cpp
class BroadcastServer {
private:
    std::shared_ptr<unilink::wrapper::TcpServer> server_;

public:
    void start() {
        server_ = unilink::tcp_server(8080)
            .on_data([this](size_t sender_id, const std::string& data) {
                // Broadcast to all clients
                std::string msg = "[Client " + std::to_string(sender_id) + "]: " + data;
                server_->send(msg);  // Send to ALL clients
                
                std::cout << "Broadcasted: " << msg << std::endl;
            })
            .build();
        
        server_->start();
    }
};
```

---

## Complete Example: Chat Server

```cpp
#include <iostream>
#include <map>
#include <mutex>
#include "unilink/unilink.hpp"

class ChatServer {
private:
    std::shared_ptr<unilink::wrapper::TcpServer> server_;
    std::map<size_t, std::string> nicknames_;
    std::mutex mutex_;

public:
    void start(uint16_t port) {
        server_ = unilink::tcp_server(port)
            .on_connect([this](size_t id, const std::string& ip) {
                std::lock_guard<std::mutex> lock(mutex_);
                nicknames_[id] = "User" + std::to_string(id);
                
                std::string msg = nicknames_[id] + " joined the chat!\n";
                server_->send(msg);
                std::cout << msg;
                
                server_->send_to_client(id, "Welcome! Type /nick <name> to set your nickname\n");
            })
            .on_data([this](size_t id, const std::string& data) {
                handle_message(id, data);
            })
            .on_disconnect([this](size_t id) {
                std::lock_guard<std::mutex> lock(mutex_);
                std::string msg = nicknames_[id] + " left the chat.\n";
                nicknames_.erase(id);
                
                server_->send(msg);
                std::cout << msg;
            })
            .build();
        
        server_->start();
        std::cout << "Chat server started on port " << port << std::endl;
    }
    
    void handle_message(size_t id, const std::string& data) {
        if (data.substr(0, 5) == "/nick") {
            // Change nickname
            std::string new_nick = data.substr(6);
            new_nick.erase(new_nick.find_last_not_of(" \n\r\t") + 1);
            
            std::lock_guard<std::mutex> lock(mutex_);
            std::string old_nick = nicknames_[id];
            nicknames_[id] = new_nick;
            
            std::string msg = old_nick + " is now known as " + new_nick + "\n";
            server_->send(msg);
        } else {
            // Broadcast message
            std::lock_guard<std::mutex> lock(mutex_);
            std::string msg = nicknames_[id] + ": " + data;
            server_->send(msg);
            std::cout << msg;
        }
    }
};

int main(int argc, char** argv) {
    uint16_t port = (argc > 1) ? std::stoi(argv[1]) : 8080;
    
    ChatServer server;
    server.start(port);
    
    std::cout << "Chat server running. Press Ctrl+C to stop." << std::endl;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
```

---

## Testing Your Chat Server

### Start Server
```bash
./chat_server 8080
```

### Connect Multiple Clients
```bash
# Terminal 1
telnet localhost 8080

# Terminal 2
telnet localhost 8080

# Terminal 3
telnet localhost 8080
```

### Try Commands
```
/nick Alice
Hello everyone!
/nick Bob
Hi Alice!
```

---

## Best Practices

### 1. Always Check Server Status
```cpp
if (!server->is_listening()) {
    std::cerr << "Server failed to start" << std::endl;
    return 1;
}
```

### 2. Use Thread-Safe Data Structures
```cpp
std::mutex mutex_;
std::map<size_t, ClientInfo> clients_;

// Always lock when accessing shared data
{
    std::lock_guard<std::mutex> lock(mutex_);
    clients_[id] = info;
}
```

### 3. Handle Graceful Shutdown
```cpp
void shutdown() {
    // Notify clients
    server_->send("Server shutting down...\n");
    
    // Wait for messages to be sent
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop server
    server_->stop();
}
```

### 4. Implement Error Recovery
```cpp
.on_error([this](const std::string& error) {
    log_error(error);
    
    // Attempt recovery if needed
    if (is_recoverable(error)) {
        attempt_recovery();
    }
})
```

---

## Common Patterns

### Pattern 1: Command Parser
```cpp
void handle_command(size_t client_id, const std::string& cmd) {
    if (cmd == "/help") {
        send_help(client_id);
    } else if (cmd == "/quit") {
        disconnect_client(client_id);
    } else if (cmd.substr(0, 5) == "/msg ") {
        send_private_message(client_id, cmd.substr(5));
    } else {
        server_->send_to_client(client_id, "Unknown command\n");
    }
}
```

### Pattern 2: Rate Limiting
```cpp
struct RateLimiter {
    size_t max_messages_per_second{10};
    std::map<size_t, std::deque<std::chrono::steady_clock::time_point>> timestamps;
    
    bool is_allowed(size_t client_id) {
        auto now = std::chrono::steady_clock::now();
        auto& times = timestamps[client_id];
        
        // Remove old timestamps
        while (!times.empty() && 
               now - times.front() > std::chrono::seconds(1)) {
            times.pop_front();
        }
        
        // Check limit
        if (times.size() >= max_messages_per_second) {
            return false;
        }
        
        times.push_back(now);
        return true;
    }
};
```

---

## Next Steps

- [Tutorial 3: Serial Communication →](03_serial_communication.md)
- [Best Practices Guide](../guides/best_practices.md)
- [Performance Tuning](../guides/performance_tuning.md)

---

## Full Example Code

✅ **Ready-to-compile examples are available!**

**Location**: `examples/tutorials/tcp_server/`

| File | Description |
|------|-------------|
| `echo_server.cpp` | Basic echo server from this tutorial |
| `chat_server.cpp` | Complete chat server with commands |

**Build and run**:
```bash
# Build echo server
cmake --build build --target tutorial_echo_server
./build/examples/tutorials/tutorial_echo_server 8080

# Build chat server
cmake --build build --target tutorial_chat_server
./build/examples/tutorials/tutorial_chat_server 8080
```

See [examples/tutorials/README.md](../../examples/tutorials/README.md) for detailed build instructions.

---

**Previous**: [← Getting Started](01_getting_started.md)  
**Next**: [Serial Communication →](03_serial_communication.md)

