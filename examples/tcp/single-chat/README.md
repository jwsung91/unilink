# TCP Chat Server and Client

Single-client TCP chat examples using the current public API.

## Server Usage

```bash
./chat_tcp_server <port>
```

Examples:

```bash
# Default port is 8080 when omitted
./chat_tcp_server

# Custom port
./chat_tcp_server 9000
```

## Client Usage

```bash
./chat_tcp_client <host> <port>
```

Examples:

```bash
# Default target is 127.0.0.1:8080 when omitted
./chat_tcp_client

# Explicit local connection
./chat_tcp_client 127.0.0.1 9000

# Remote connection
./chat_tcp_client 192.168.1.100 9000
```

## Behavior

- The server runs in `.single_client()` mode.
- Client messages are printed on the server console.
- The server operator can type messages and send them back.
- The client sends stdin lines and exits on `/quit`.

## Expected Output

Server:

```text
[INFO] [server] [main] Server started on port 8080
[INFO] [server] [STATE] Client connected: 127.0.0.1:...
[Client] hello
```

Client:

```text
[INFO] [client] [STATE] Connected
[Server] hello back
```

## Quick Test

```bash
# Terminal 1
./chat_tcp_server

# Terminal 2
./chat_tcp_client
```

## Notes

- This example is intentionally single-client.
- For concurrent clients, see `examples/tcp/multi-chat/`.
- Both binaries use the public `unilink/unilink.hpp` API.

## Troubleshooting

### Port Already in Use

```bash
netstat -tulpn | grep :9000
```

### Connection Issues

- Verify the server is already listening.
- Confirm the client host and port.
- Check firewall settings for remote access.
