# UDP Examples

## Examples Structure

- `sync/` — Synchronous (blocking) examples. Uses `start_sync()` for simplicity.
- `async/` — Asynchronous (callback-driven) examples.

## Binaries

| Binary | Description |
|--------|-------------|
| `sync_udp_receiver` | Blocking UDP receiver |
| `sync_udp_sender` | Blocking UDP sender |
| `async_udp_receiver`| Event-driven UDP receiver |
| `async_udp_sender`| Non-blocking UDP sender |

## Usage

```bash
# Terminal 1 — receiver (default port 9000)
./sync_udp_receiver [port]

# Terminal 2 — sender (default target 127.0.0.1:9000)
./sync_udp_sender [host] [port]
```

Type lines in the sender terminal. Each line is sent immediately and printed by the receiver. `/quit` exits the sender.

## API Patterns

- `udp_client(local_port).on_data(...).auto_start(true).build()` — bind and receive
- `udp_client(0).remote(host, port).build()` + `start_sync()` — send to a fixed endpoint
