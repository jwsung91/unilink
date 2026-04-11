# TCP Echo Server and Client

TCP echo examples using the current builder and wrapper API.

## Server Usage

```bash
./echo_tcp_server <port>
```

Examples:

```bash
# Default port is 8080 when omitted
./echo_tcp_server

# Custom port
./echo_tcp_server 9000
```

## Client Usage

```bash
./echo_tcp_client <host> <port>
```

Examples:

```bash
# Default target is 127.0.0.1:8080 when omitted
./echo_tcp_client

# Explicit local connection
./echo_tcp_client 127.0.0.1 9000

# Remote connection
./echo_tcp_client 192.168.1.100 9000
```

## Behavior

- The server binds a TCP port in single-client mode.
- Each message received from the client is echoed back to that client.
- The client reads stdin lines, sends them, and prints echoed responses.
- Exit with `/quit` or `Ctrl+C`.

## Expected Output

Server:

```text
[INFO] [server] [startup] Starting server on port 8080...
[INFO] [server] [startup] Server started successfully. Waiting for client connections...
[INFO] [server] [connect] Client 1 connected: 127.0.0.1:...
[INFO] [server] [data] Client 1 message: hello
[INFO] [server] [echo] Echoed to client 1
```

Client:

```text
[INFO] [client] [startup] Connecting to 127.0.0.1:8080...
[INFO] [client] [connect] Connected to server
[INFO] [client] [send] Sent: hello
[INFO] [client] [data] Received: hello
```

## Quick Test

```bash
# Terminal 1
./echo_tcp_server

# Terminal 2
./echo_tcp_client
```

## Notes

- The server example uses `.single_client()`.
- The client example uses retry settings through the builder API.
- Both examples are built from the public `unilink/unilink.hpp` interface.

## Troubleshooting

### Port Already in Use

```bash
netstat -tulpn | grep :9000
# or
lsof -i :9000
```

Start the server on another port if needed:

```bash
./echo_tcp_server 9001
```

### Connection Refused

- Verify the server is running.
- Check the host and port.
- Check firewall rules if connecting across machines.
