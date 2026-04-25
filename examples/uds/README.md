# UDS Examples (Unix Domain Socket)

Unix domain socket examples for local IPC. Requires Linux or macOS.

## Binaries

| Binary | Description |
|--------|-------------|
| `uds_echo_server` | Accepts unlimited clients, echoes each message back to the sender |
| `uds_echo_client` | Connects to a UDS server and enters an interactive send/receive loop |

## Usage

```bash
# Terminal 1
./uds_echo_server [socket_path]

# Terminal 2
./uds_echo_client [socket_path]
```

Default socket path: `/tmp/unilink_echo.sock`. Type messages in the client; `/quit` disconnects.

## Notes

- If a stale socket file is left behind after a crash, remove it before restarting the server.
- The API mirrors the TCP examples: `uds_server(path)` / `uds_client(path)` instead of `tcp_server(port)` / `tcp_client(host, port)`.
