# TCP Echo Server and Client

TCP echo server and client for network communication testing. The server echoes back any data received from clients.

## Server Usage

```bash
./echo_tcp_server <port>
```

### Examples
```bash
# Default port 9000
./echo_tcp_server 9000

# Custom port
./echo_tcp_server 8080
```

## Client Usage

```bash
./echo_tcp_client <host> <port>
```

### Examples
```bash
# Local connection
./echo_tcp_client 127.0.0.1 9000

# Remote connection
./echo_tcp_client 192.168.1.100 9000
```

## What it does

### Server
- Listens on the specified port
- Accepts client connections
- Echoes received data back to clients
- Logs connection status and data flow
- Handles multiple clients (one at a time)

### Client
- Connects to the specified server
- Sends data to the server
- Receives echoed data from the server
- Logs connection status and data flow

## Expected Output

### Server Output
```
2025-01-15 10:30:45.123 [INFO] [server] [start] Starting TCP server on port 9000
2025-01-15 10:30:45.124 [INFO] [server] [STATE] Client connected
2025-01-15 10:30:45.125 [INFO] [server] [RX] Hello from client
2025-01-15 10:30:45.126 [INFO] [server] [TX] Hello from client
2025-01-15 10:30:45.127 [INFO] [server] [STATE] Client disconnected
```

### Client Output
```
2025-01-15 10:30:45.123 [INFO] [client] [start] Connecting to 127.0.0.1:9000
2025-01-15 10:30:45.124 [INFO] [client] [STATE] Connected
2025-01-15 10:30:45.125 [INFO] [client] [TX] Hello from client
2025-01-15 10:30:45.126 [INFO] [client] [RX] Hello from client
```

## Testing Workflow

### Local Testing
1. **Start the server**:
   ```bash
   ./echo_tcp_server 9000
   ```

2. **Start the client** (in another terminal):
   ```bash
   ./echo_tcp_client 127.0.0.1 9000
   ```

3. **Send messages**:
   - Type messages in the client
   - Server will echo them back
   - Observe the logging output

### Network Testing
1. **Start server on machine A**:
   ```bash
   ./echo_tcp_server 9000
   ```

2. **Start client on machine B**:
   ```bash
   ./echo_tcp_client <machine_A_IP> 9000
   ```

## Features

### Server Features
- **Port binding**: Listens on specified port
- **Client handling**: Accepts and manages client connections
- **Data echoing**: Sends received data back to client
- **Connection logging**: Logs all connection events
- **Error handling**: Graceful handling of connection errors

### Client Features
- **Connection management**: Connects to specified server
- **Data transmission**: Sends data to server
- **Data reception**: Receives echoed data from server
- **Status logging**: Logs connection and data events
- **Error handling**: Handles connection failures

## Use Cases

- **Network testing**: Verify network connectivity and latency
- **Protocol testing**: Test custom protocols over TCP
- **Load testing**: Test server performance under load
- **Debugging**: Debug network communication issues

## Troubleshooting

### Port Already in Use
```bash
# Check what's using the port
netstat -tulpn | grep :9000
# or
lsof -i :9000

# Use a different port
./echo_tcp_server 9001
```

### Connection Refused
- Verify server is running
- Check firewall settings
- Ensure correct IP address and port
- Check if port is accessible

### Firewall Issues
```bash
# Linux - allow port through firewall
sudo ufw allow 9000

# Check firewall status
sudo ufw status
```
