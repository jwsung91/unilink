# Serial Example

## Binary

| Binary | Description |
|--------|-------------|
| `sync_serial_echo` | Opens a serial port, prints received data, sends user input |

## Examples Structure

- `sync/` — Synchronous (blocking) examples. Uses `start_sync()` for simplicity.
- `async/` — Asynchronous (callback-driven) examples (Coming Soon).

## Usage

```bash
./sync_serial_echo [device] [baud]
```

Defaults: `/dev/ttyUSB0` at `115200` baud. Type messages; `/quit` exits.

## Testing Without Hardware

Use `socat` to create a virtual serial pair:

```bash
# Terminal 1 — create virtual pair
socat -d -d pty,raw,echo=0,link=/tmp/ttyA pty,raw,echo=0,link=/tmp/ttyB

# Terminal 2 — run serial echo
./sync_serial_echo /tmp/ttyA 115200

# Terminal 3 — interact
socat - /tmp/ttyB
```

## Notes

- Serial reconnects automatically if the device disappears and reappears — this is intentional.
- Linux: grant device access with `sudo usermod -a -G dialout $USER` (re-login required).
- Windows: use `COM3`, `COM4`, etc.
