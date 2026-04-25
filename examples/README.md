# Unilink Examples

Runnable examples for the `unilink/unilink.hpp` public API.

## Structure

```
examples/
  tcp/     — TCP echo and broadcast examples
  udp/     — UDP sender and receiver
  uds/     — Unix Domain Socket echo examples (Linux/macOS)
  serial/  — Serial port echo example (Linux/macOS)
```

## Build

```bash
cmake -S . -B build
cmake --build build
# Binaries land in build/bin/
```

## TCP

```bash
# Terminal 1 — echo server (default port 8080)
./build/bin/tcp_echo_server [port]

# Terminal 2 — client
./build/bin/tcp_echo_client [host] [port]

# Multi-client broadcast server
./build/bin/tcp_broadcast_server [port]
```

## UDP

```bash
# Terminal 1 — receiver (default port 9000)
./build/bin/udp_receiver [port]

# Terminal 2 — sender
./build/bin/udp_sender [host] [port]
```

## UDS (Unix Domain Socket)

```bash
# Terminal 1
./build/bin/uds_echo_server [socket_path]

# Terminal 2
./build/bin/uds_echo_client [socket_path]
```

Default socket path: `/tmp/unilink_echo.sock`

## Serial

```bash
./build/bin/serial_echo [device] [baud]
```

Default: `/dev/ttyUSB0` at `115200`. Use `socat` to create virtual ports for testing:

```bash
# Terminal 1 — create virtual serial pair
socat -d -d pty,raw,echo=0,link=/tmp/ttyA pty,raw,echo=0,link=/tmp/ttyB

# Terminal 2 — run serial echo on first port
./build/bin/serial_echo /tmp/ttyA 115200

# Terminal 3 — interact via second port
socat - /tmp/ttyB
```

## Platform Notes

- UDS and Serial require Linux or macOS
- Serial device access on Linux typically requires `dialout` group membership:
  `sudo usermod -a -G dialout $USER`
- Windows: use `COM3`, `COM4` etc. for serial; UDS is unavailable
