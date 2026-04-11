# Serial Chat Example

Interactive serial chat example using the current public API.

## Usage

```bash
./chat_serial <device> <baud_rate>
```

Examples:

```bash
# Linux
./chat_serial /dev/ttyUSB0 115200

# Windows
./chat_serial COM3 9600
```

If arguments are omitted, the example defaults to `/dev/ttyUSB0` and `115200`.

## Behavior

- Opens the configured serial device
- Prints connection and error events
- Sends each stdin line over the serial link
- Prints received data as it arrives
- Exits on `/quit`

## Expected Output

```text
Serial Chat started. Type messages to send.
Type '/quit' to exit.
> hello
[RX] hello back
```

## Quick Test With `socat`

```bash
# Terminal 1
socat -d -d pty,raw,echo=0,link=/tmp/ttyA pty,raw,echo=0,link=/tmp/ttyB

# Terminal 2
./chat_serial /tmp/ttyA 115200

# Terminal 3
./chat_serial /tmp/ttyB 115200
```

## Notes

- The example uses `serial(...).on_connect(...).on_disconnect(...).on_data(...).on_error(...).build()`.
- The prompt is printed locally; incoming data is shown as `[RX] ...`.
- For a simpler send/echo flow, use `examples/serial/echo/`.

## Troubleshooting

### No Messages Received

- Check that the peer process is connected and running
- Verify both sides use the same baud rate
- Confirm the correct device path

### Permission Denied

```bash
sudo usermod -a -G dialout $USER
```
