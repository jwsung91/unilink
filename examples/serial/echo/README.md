# Serial Echo Example

Serial echo example using the current public API.

## Usage

```bash
./echo_serial <device> <baud_rate>
```

Examples:

```bash
# Linux
./echo_serial /dev/ttyUSB0 115200

# Windows
./echo_serial COM3 9600
```

If arguments are omitted, the example defaults to `/dev/ttyUSB0` and `115200`.

## Behavior

- Opens the configured serial device
- Logs connect, disconnect, RX, and TX events
- Sends each line you type to the serial link
- Prints incoming data and echoes it back if the peer reflects it
- Exits when you submit an empty line

## Expected Output

```text
[INFO] [serial] [main] Serial started successfully
[INFO] [serial] [STATE] Serial device connected
[INFO] [serial] [TX] hello
[INFO] [serial] [RX] hello
```

## Quick Test With `socat`

```bash
# Terminal 1
socat -d -d pty,raw,echo=0,link=/tmp/ttyA pty,raw,echo=0,link=/tmp/ttyB

# Terminal 2
./echo_serial /tmp/ttyA 115200

# Terminal 3
./echo_serial /tmp/ttyB 115200
```

## Notes

- The example uses `serial(...).on_connect(...).on_data(...).on_error(...).build()`.
- It waits on `start().get()` before entering the send loop.
- For interactive two-way chatting, use `examples/serial/chat/`.

## Troubleshooting

### Device Not Found

```bash
ls /dev/tty*
```

### Permission Denied

```bash
sudo usermod -a -G dialout $USER
```

### No Traffic

- Verify both sides are using the same baud rate
- Confirm the device is not already opened by another program
- Check adapter and cable wiring
