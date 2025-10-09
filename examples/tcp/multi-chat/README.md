# Multi-Client TCP Chat Server and Client

Advanced TCP chat system supporting multiple simultaneous clients. The server broadcasts messages from one client to all connected clients.

## Server Usage

```bash
./multi_chat_tcp_server <port>
```

### Examples
```bash
# Default port 9000
./multi_chat_tcp_server 9000

# Custom port
./multi_chat_tcp_server 8080
```

## Client Usage

```bash
./multi_chat_tcp_client <host> <port>
```

### Examples
```bash
# Local connection
./multi_chat_tcp_client 127.0.0.1 9000

# Remote connection
./multi_chat_tcp_client 192.168.1.100 9000
```

## What it does

### Server
- Listens on the specified port
- Accepts multiple simultaneous client connections
- Broadcasts messages from one client to all connected clients
- Manages client connections and disconnections
- Logs all connection events and messages

### Client
- Connects to the specified server
- Sends messages to the server
- Receives messages from all other clients
- Displays messages with sender information
- Handles connection status and errors

## Expected Output

### Server Output
```
2025-01-15 10:30:45.123 [INFO] [server] [start] Starting multi-chat server on port 9000
2025-01-15 10:30:45.124 [INFO] [server] [STATE] Client 1 connected
2025-01-15 10:30:45.125 [INFO] [server] [STATE] Client 2 connected
2025-01-15 10:30:45.126 [INFO] [server] [RX] [Client 1] Hello everyone!
2025-01-15 10:30:45.127 [INFO] [server] [BROADCAST] Broadcasting to 2 clients
2025-01-15 10:30:45.128 [INFO] [server] [STATE] Client 1 disconnected
```

### Client Output
```
2025-01-15 10:30:45.123 [INFO] [client] [start] Connecting to 127.0.0.1:9000
2025-01-15 10:30:45.124 [INFO] [client] [STATE] Connected
2025-01-15 10:30:45.125 [INFO] [client] [RX] [Client 1] Hello everyone!
2025-01-15 10:30:45.126 [INFO] [client] [TX] Hello back!
```

## Testing Workflow

### Multi-Client Testing
1. **Start the server**:
   ```bash
   ./multi_chat_tcp_server 9000
   ```

2. **Start multiple clients** (in separate terminals):
   ```bash
   # Terminal 1
   ./multi_chat_tcp_client 127.0.0.1 9000
   
   # Terminal 2
   ./multi_chat_tcp_client 127.0.0.1 9000
   
   # Terminal 3
   ./multi_chat_tcp_client 127.0.0.1 9000
   ```

3. **Start chatting**:
   - Type messages in any client
   - All other clients will receive the messages
   - Messages are tagged with sender information

### Network Testing
1. **Start server on machine A**:
   ```bash
   ./multi_chat_tcp_server 9000
   ```

2. **Start clients on different machines**:
   ```bash
   # Machine B
   ./multi_chat_tcp_client <machine_A_IP> 9000
   
   # Machine C
   ./multi_chat_tcp_client <machine_A_IP> 9000
   ```

## Features

### Server Features
- **Multi-client support**: Handles multiple simultaneous connections
- **Message broadcasting**: Sends messages from one client to all others
- **Client management**: Tracks connected clients and their status
- **Connection logging**: Logs all connection and disconnection events
- **Message logging**: Logs all messages and broadcast events
- **Signal handling**: Graceful shutdown with Ctrl+C

### Client Features
- **Connection management**: Connects to the multi-chat server
- **Message sending**: Sends messages to the server
- **Message reception**: Receives messages from all other clients
- **Sender identification**: Messages show which client sent them
- **Status logging**: Logs connection and message events
- **Error handling**: Handles connection failures gracefully

## Advanced Features

### Client Identification
- Each client is assigned a unique ID
- Messages are tagged with sender information
- Server tracks client connections and disconnections

### Message Broadcasting
- Server receives message from one client
- Broadcasts to all other connected clients
- Excludes the sender from receiving their own message

### Connection Management
- Automatic client disconnection handling
- Server continues running when clients disconnect
- New clients can join at any time

## Use Cases

- **Group chat**: Multi-user chat application
- **Team communication**: Internal team chat system
- **Remote collaboration**: Chat between team members
- **Broadcasting**: Send messages to multiple recipients
- **Real-time communication**: Instant messaging system

## Performance Considerations

### Server Capacity
- Server can handle multiple clients simultaneously
- Performance depends on system resources
- Consider connection limits for production use

### Network Bandwidth
- All messages are broadcast to all clients
- Consider bandwidth usage for large groups
- Implement message filtering if needed

## Troubleshooting

### Client Connection Issues
- Verify server is running
- Check firewall settings
- Ensure correct IP address and port
- Check network connectivity

### Message Not Received
- Verify client is connected to server
- Check if other clients are connected
- Ensure server is broadcasting messages
- Check for network issues

### Server Performance
- Monitor server resource usage
- Check connection limits
- Consider server capacity for large groups
- Implement connection pooling if needed
