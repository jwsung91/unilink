# TCP Communication Examples

TCP examples using the current public API.

## Included Examples

- `single-echo/`: Single-client echo server and client
- `single-chat/`: Single-client chat server and client
- `multi-chat/`: Multi-client chat server and client

## Common Usage

```bash
# Server
./server_binary <port>

# Client
./client_binary <host> <port>
```

Most binaries also provide sensible defaults when arguments are omitted:

- Server examples default to port `8080`
- Client examples default to `127.0.0.1:8080`

## Quick Start

### Echo

```bash
# Terminal 1
./echo_tcp_server

# Terminal 2
./echo_tcp_client
```

### Single-Client Chat

```bash
# Terminal 1
./chat_tcp_server

# Terminal 2
./chat_tcp_client
```

### Multi-Client Chat

```bash
# Terminal 1
./multi_chat_tcp_server

# Terminal 2
./multi_chat_tcp_client

# Terminal 3
./multi_chat_tcp_client
```

## Notes

- These examples use the builder and wrapper API from `unilink/unilink.hpp`.
- `single-echo` uses `.single_client()` and targeted replies with `send_to(...)`.
- `single-chat` is intentionally single-client.
- `multi-chat` uses `.unlimited_clients()` and `broadcast(...)`.

## Troubleshooting

### Port Already in Use

```bash
netstat -tulpn | grep :9000
# or
lsof -i :9000
```

Run the server on another port if needed:

```bash
./echo_tcp_server 9001
```

### Connection Refused

- Verify the server is already running
- Check the client host and port
- Confirm firewall settings for remote connections
