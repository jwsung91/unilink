# Serial Communication Examples

Serial examples using the current public API.

## Included Examples

- `echo/`: Echoes each received line back to the same serial link
- `chat/`: Interactive two-way serial chat

## Common Usage

Both examples accept the same arguments:

```bash
./echo_serial <device> <baud_rate>
./chat_serial <device> <baud_rate>
```

Examples:

```bash
# Linux
./echo_serial /dev/ttyUSB0 115200
./chat_serial /dev/ttyUSB0 115200

# Windows
./echo_serial COM3 9600
./chat_serial COM3 9600
```

If arguments are omitted, the code defaults to `/dev/ttyUSB0` and `115200`.

## Quick Test With `socat`

```bash
# Terminal 1
socat -d -d pty,raw,echo=0,link=/tmp/ttyA pty,raw,echo=0,link=/tmp/ttyB

# Terminal 2
./echo_serial /tmp/ttyA 115200

# Terminal 3
./echo_serial /tmp/ttyB 115200
```

For interactive chat instead of echo:

```bash
# Terminal 2
./chat_serial /tmp/ttyA 115200

# Terminal 3
./chat_serial /tmp/ttyB 115200
```

## Notes

- These examples use `serial(...).on_connect(...).on_data(...).on_error(...).build()`.
- `echo_serial` exits when you submit an empty line.
- `chat_serial` exits on `/quit`.
- On Linux, serial-device access often requires `dialout` or similar group membership.

## Troubleshooting

### Permission Denied

```bash
sudo usermod -a -G dialout $USER
```

Log out and back in after changing group membership.

### Device Not Found

- Check available devices with `ls /dev/tty*`
- Verify the correct device path such as `/dev/ttyUSB0` or `/dev/ttyACM0`
- On Windows, verify the COM port in Device Manager

### Connection Issues

- Make sure baud rates match on both sides
- Verify another program is not already using the port
- Recheck cable, adapter, and device wiring
