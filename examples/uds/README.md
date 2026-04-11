# UDS Echo Examples

Unix domain socket examples using the current wrapper and builder API.

## Binaries

- `echo_uds_server`
- `echo_uds_client`

## Usage

```bash
./echo_uds_server [socket_path]
./echo_uds_client [socket_path]
```

If `socket_path` is omitted, both examples use `/tmp/unilink_echo.sock`.

## Quick Start

**Terminal 1**

```bash
cd examples/uds
./echo_uds_server
```

**Terminal 2**

```bash
cd examples/uds
./echo_uds_client
```

Type lines in the client terminal. The server prints each message and echoes it back to the same client.

## Notes

- These examples target Unix-like platforms where UDS is available.
- The source uses `unilink::uds_server(...)` and `unilink::uds_client(...)` from `unilink/unilink.hpp`.
- If an old socket file is left behind after a crash, remove it before restarting the server.
