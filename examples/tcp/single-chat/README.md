# TCP Chat Server and Client

TCP chat server and client for network-based chat communication. The server displays messages from clients, and clients can send messages to the server.

## Server Usage

```bash
./chat_tcp_server <port>
```

### Examples
```bash
# Default port 9000
./chat_tcp_server 9000

# Custom port
./chat_tcp_server 8080
```

## Client Usage

```bash
./chat_tcp_client <host> <port>
```

### Examples
```bash
# Local connection
./chat_tcp_client 127.0.0.1 9000

# Remote connection
./chat_tcp_client 192.168.1.100 9000
```

## What it does

### Server
- Listens on the specified port
- Accepts client connections
- Displays received messages from clients
- Logs connection status and messages
- Handles client disconnections

### Client
- Connects to the specified server
- Sends typed messages to the server
- Receives and displays messages from the server
- Logs connection status and messages

## Expected Output

### Server Output
```
2025-01-15 10:30:45.123 [INFO] [server] [start] Starting TCP server on port 9000
2025-01-15 10:30:45.124 [INFO] [server] [STATE] Client connected
2025-01-15 10:30:45.125 [INFO] [server] [RX] Hello from client!
2025-01-15 10:30:45.126 [INFO] [server] [TX] Server response
2025-01-15 10:30:45.127 [INFO] [server] [STATE] Client disconnected
```

### Client Output
```
2025-01-15 10:30:45.123 [INFO] [client] [start] Connecting to 127.0.0.1:9000
2025-01-15 10:30:45.124 [INFO] [client] [STATE] Connected
2025-01-15 10:30:45.125 [INFO] [client] [TX] Hello from client!
2025-01-15 10:30:45.126 [INFO] [client] [RX] Server response
```

## Testing Workflow

### Local Testing
1. **Start the server**:
   ```bash
   ./chat_tcp_server 9000
   ```

2. **Start the client** (in another terminal):
   ```bash
   ./chat_tcp_client 127.0.0.1 9000
   ```

3. **Start chatting**:
   - Type messages in the client and press Enter
   - Server will display received messages
   - Server can also send messages to client

### Network Testing
1. **Start server on machine A**:
   ```bash
   ./chat_tcp_server 9000
   ```

2. **Start client on machine B**:
   ```bash
   ./chat_tcp_client <machine_A_IP> 9000
   ```

3. **Chat between machines**:
   - Send messages from client to server
   - Server displays messages and can respond

## Features

### Server Features
- **Port binding**: Listens on specified port
- **Client management**: Accepts and manages client connections
- **Message display**: Shows received messages from clients
- **Interactive input**: Server can type and send messages to clients
- **Connection logging**: Logs all connection and message events
- **Signal handling**: Graceful shutdown with Ctrl+C

### Client Features
- **Connection management**: Connects to specified server
- **Interactive chat**: Type messages and send to server
- **Message reception**: Receives and displays messages from server
- **Status logging**: Logs connection and message events
- **Error handling**: Handles connection failures gracefully

## Use Cases

- **Network chat**: Simple chat application over network
- **Remote communication**: Chat between different machines
- **Protocol testing**: Test custom chat protocols
- **Debugging**: Debug network communication issues
- **Remote control**: Send commands to remote systems

## Advanced Usage

### Multiple Clients
The server can handle multiple clients, but only one at a time in this implementation. For true multi-client support, see the multi-chat example in the examples/tcp/multi-chat directory.

### Custom Messages
- Server can send custom messages to clients
- Clients receive and display all server messages
- Both sides can initiate conversations

## Troubleshooting

### Port Already in Use
```bash
# Check what's using the port
netstat -tulpn | grep :9000
# Use a different port
./chat_tcp_server 9001
```

### Connection Issues
- Verify server is running
- Check firewall settings
- Ensure correct IP address and port
- Check network connectivity

### Message Not Received
- Verify connection is established
- Check if messages are being sent
- Ensure both sides are running properly
- Check for network issues
