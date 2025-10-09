# TCP Communication Examples

Examples demonstrating TCP network communication using the unilink library.

## Examples

- **single-echo/**: TCP echo server and client for network echo testing
- **single-chat/**: TCP chat server and client for network chat

## Common Usage

```bash
# Start server
./server_example <port>

# Connect client
./client_example <host> <port>
```

## Network Configuration

### Local Testing
```bash
# Server
./echo_tcp_server 9000

# Client (same machine)
./echo_tcp_client 127.0.0.1 9000
```

### Remote Testing
```bash
# Server
./echo_tcp_server 9000

# Client (different machine)
./echo_tcp_client 192.168.1.100 9000
```

## Prerequisites

- Network connectivity between server and client
- Available port (not in use by other applications)
- Firewall configuration (if needed)

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

### Firewall Issues
```bash
# Linux - allow port through firewall
sudo ufw allow 9000

# Check firewall status
sudo ufw status
```

## Example Workflows

### Single Echo Server/Client
1. Start server: `./echo_tcp_server 9000`
2. Start client: `./echo_tcp_client 127.0.0.1 9000`
3. Type messages in client
4. Server echoes messages back

### Single Chat Server/Client
1. Start server: `./chat_tcp_server 9000`
2. Start client: `./chat_tcp_client 127.0.0.1 9000`
3. Type messages in client
4. Server displays received messages
