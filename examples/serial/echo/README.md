# Serial Echo Server

A serial echo server that receives data from a serial port and echoes it back to the sender.

## Usage

```bash
./echo_serial <device> <baud_rate>
```

### Examples
```bash
# Linux
./echo_serial /dev/ttyUSB0 115200

# Windows
./echo_serial COM3 9600

# macOS
./echo_serial /dev/tty.usbserial-* 115200
```

## What it does

- Connects to the specified serial device
- Automatically retries connection if it fails
- Logs connection status and data flow
- Echoes received data back to the sender
- Handles connection drops and reconnection

## Expected Output

```
2025-01-15 10:30:45.123 [INFO] [serial] [start] Starting device: /dev/ttyUSB0
2025-01-15 10:30:45.124 [INFO] [serial] [STATE] Serial device connected
2025-01-15 10:30:45.125 [INFO] [serial] [TX] SER 0
2025-01-15 10:30:45.126 [INFO] [serial] [RX] SER 0
2025-01-15 10:30:45.127 [INFO] [serial] [TX] SER 1
2025-01-15 10:30:45.128 [INFO] [serial] [RX] SER 1
```

## Testing

### Using socat (Virtual Serial Ports)
```bash
# Terminal 1: Create virtual serial pair
socat -d -d pty,raw,echo=0,link=/tmp/ttyA pty,raw,echo=0,link=/tmp/ttyB
# Creates: /tmp/ttyA <-> /tmp/ttyB

# Terminal 2: Run echo server
./echo_serial /tmp/ttyA 115200

# Terminal 3: Connect to second port
./echo_serial /tmp/ttyB 115200

# Type messages and see them echoed back
```

### Using a Serial Terminal
1. Connect a serial device (Arduino, USB-to-Serial adapter, etc.)
2. Run the echo server: `./echo_serial /dev/ttyUSB0 115200`
3. Open a serial terminal (minicom, screen, etc.) on the same device
4. Send data from the terminal
5. Observe the echoed data

### Using Another Serial Device
1. Connect two serial devices
2. Run echo server on one device
3. Send data from the other device
4. Verify data is echoed back

## Features

- **Automatic reconnection**: Retries connection every 5 seconds if disconnected
- **Connection status logging**: Clear indication of connection state
- **Data logging**: All transmitted and received data is logged
- **Error handling**: Graceful handling of connection errors
- **Configurable parameters**: Device path and baud rate

## Troubleshooting

### Device Not Found
```bash
# Check available devices
ls /dev/tty*

# Check permissions
ls -l /dev/ttyUSB0
```

### Permission Denied
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER
# Log out and log back in
```

### Connection Issues
- Verify device is not in use by another application
- Check baud rate matches device settings
- Ensure cable and device are working properly
