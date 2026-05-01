# UDS Examples (Unix Domain Socket)

Unix domain socket examples for local IPC. Requires Linux or macOS.

## Examples Structure

- `sync/` — Synchronous (blocking) examples. Uses `start_sync()` for simplicity.
- `async/` — Asynchronous (callback-driven) examples. Shows non-blocking patterns.

## Binaries

| Binary | Description |
|--------|-------------|
| `sync_uds_echo_server` | Blocking UDS echo server |
| `sync_uds_echo_client` | Interactive blocking UDS client |
| `async_uds_echo_server`| Event-driven UDS server |
| `async_uds_echo_client`| Event-driven UDS client |

## Usage

```bash
# Terminal 1
./sync_uds_echo_server [socket_path]

# Terminal 2
./sync_uds_echo_client [socket_path]
```

Default socket path: `/tmp/unilink_echo.sock`. Type messages in the client; `/quit` disconnects.

## Notes

- If a stale socket file is left behind after a crash, remove it before restarting the server.
- The API mirrors the TCP examples: `uds_server(path)` / `uds_client(path)` instead of `tcp_server(port)` / `tcp_client(host, port)`.
