# TCP Examples

## Examples Structure

- `sync/` — Synchronous (blocking) examples. Uses `start_sync()` for simplicity.
- `async/` — Asynchronous (callback-driven) examples. Shows non-blocking patterns.

## Binaries

| Binary | Description |
|--------|-------------|
| `sync_tcp_echo_server` | Blocking echo server |
| `sync_tcp_echo_client` | Interactive blocking client |
| `async_tcp_echo_server`| Event-driven server with status heartbeat |
| `async_tcp_echo_client`| Event-driven client with parallel input handling |

## Usage

```bash
# Echo pair (default port 8080)
./sync_tcp_echo_server [port]
./sync_tcp_echo_client [host] [port]

# Broadcast server — connect multiple clients to see messages relayed
./sync_tcp_broadcast_server [port]
./sync_tcp_echo_client 127.0.0.1 8080   # connect as many as you like
```

Type messages in any client terminal. `/quit` disconnects.

## API Patterns

- `send_to(client_id, data)` — reply to a specific client (echo server)
- `broadcast(data)` — send to all connected clients (broadcast server)
- `start_sync()` — block until the server is listening or failed

## Troubleshooting

```bash
# Check if a port is in use
ss -tlnp | grep :8080

# Run on a different port
./tcp_echo_server 9001
./tcp_echo_client 127.0.0.1 9001
```
