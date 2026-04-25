# TCP Examples

## Binaries

| Binary | Description |
|--------|-------------|
| `tcp_echo_server` | Accepts unlimited clients, echoes each message back to the sender |
| `tcp_echo_client` | Connects to a server and enters an interactive send/receive loop |
| `tcp_broadcast_server` | Accepts unlimited clients, broadcasts every message to all of them |

## Usage

```bash
# Echo pair (default port 8080)
./tcp_echo_server [port]
./tcp_echo_client [host] [port]

# Broadcast server — connect multiple clients to see messages relayed
./tcp_broadcast_server [port]
./tcp_echo_client 127.0.0.1 8080   # connect as many as you like
```

Type messages in any client terminal. `/quit` disconnects.

## API Patterns

- `unlimited_clients()` — allow any number of concurrent connections
- `send_to(client_id, data)` — reply to a specific client (echo server)
- `broadcast(data)` — send to all connected clients (broadcast server)
- `start().get()` — block until the server is listening or failed

## Troubleshooting

```bash
# Check if a port is in use
ss -tlnp | grep :8080

# Run on a different port
./tcp_echo_server 9001
./tcp_echo_client 127.0.0.1 9001
```
