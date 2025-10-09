# Serial Chat Application

An interactive serial chat application that allows real-time communication through a serial port.

## Usage

```bash
./chat_serial <device> <baud_rate>
```

### Examples
```bash
# Linux
./chat_serial /dev/ttyUSB0 115200

# Windows
./chat_serial COM3 9600

# macOS
./chat_serial /dev/tty.usbserial-* 115200
```

## What it does

- Connects to the specified serial device
- Provides an interactive chat interface
- Sends typed messages to the serial port
- Displays received messages from the serial port
- Handles connection status and errors

## Expected Output

```
2025-01-15 10:30:45.123 [INFO] [serial] [start] Starting device: /dev/ttyUSB0
2025-01-15 10:30:45.124 [INFO] [serial] [STATE] Serial device connected
Hello, this is a test message
2025-01-15 10:30:45.125 [INFO] [serial] [TX] Hello, this is a test message
2025-01-15 10:30:45.126 [INFO] [serial] [RX] Response from device
```

## How to use

1. **Start the application**:
   ```bash
   ./chat_serial /dev/ttyUSB0 115200
   ```

2. **Wait for connection**:
   - The application will attempt to connect to the serial device
   - You'll see connection status messages

3. **Start chatting**:
   - Type messages and press Enter to send
   - Received messages will be displayed automatically
   - Type `Ctrl+C` to exit

## Testing with socat (Virtual Serial Ports)

### Setup Virtual Serial Ports
```bash
# Terminal 1: Create virtual serial pair
socat -d -d pty,raw,echo=0,link=/tmp/ttyA pty,raw,echo=0,link=/tmp/ttyB
# Creates: /tmp/ttyA <-> /tmp/ttyB

# Terminal 2: Run chat application on first port
./chat_serial /tmp/ttyA 115200

# Terminal 3: Connect to second port
socat - /tmp/ttyB
# Start chatting between terminals
```

## Testing with Two Devices

### Setup
1. Connect two serial devices (or use USB-to-Serial adapters)
2. Run chat application on both devices with different device paths

### Example
```bash
# Device 1
./chat_serial /dev/ttyUSB0 115200

# Device 2  
./chat_serial /dev/ttyUSB1 115200
```

## Features

- **Interactive interface**: Real-time message exchange
- **Connection monitoring**: Clear indication of connection status
- **Error handling**: Graceful handling of connection issues
- **Message logging**: All sent and received messages are logged
- **Automatic reconnection**: Retries connection if disconnected

## Use Cases

- **Device communication**: Chat with embedded devices
- **Debugging**: Interactive communication for testing
- **Remote control**: Send commands to remote devices
- **Data exchange**: Transfer data between devices

## Troubleshooting

### No Messages Received
- Check if the other device is connected and running
- Verify baud rate matches on both devices
- Ensure cables are properly connected

### Connection Issues
- Verify device path is correct
- Check device permissions
- Ensure device is not in use by another application

### Permission Denied
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER
# Log out and log back in
```
