# Serial Communication Examples

Examples demonstrating serial communication using the unilink library.

## Examples

- **echo/**: Serial echo server that echoes received data back to sender
- **chat/**: Serial chat application for interactive communication

## Common Usage

```bash
# Set serial port permissions (Linux)
sudo chmod 666 /dev/ttyUSB0

# Windows uses COM ports
# COM3, COM4, etc.
```

## Testing with socat (Virtual Serial Ports)

For testing without physical serial devices, you can use socat to create virtual serial port pairs:

### Setup Virtual Serial Ports
```bash
# Terminal 1: Create virtual serial pair
socat -d -d pty,raw,echo=0,link=/tmp/ttyA pty,raw,echo=0,link=/tmp/ttyB
# Creates: /tmp/ttyA <-> /tmp/ttyB
```

### Test Echo Server
```bash
# Terminal 2: Run echo server on first port
cd serial/echo
./echo_serial /tmp/ttyA 115200

# Terminal 3: Connect to second port
cd serial/echo
./echo_serial /tmp/ttyB 115200

# Type messages and see them echoed back
```

### Test Chat Application
```bash
# Terminal 2: Run chat server on first port
cd serial/chat
./chat_serial /tmp/ttyA 115200

# Terminal 3: Connect to second port
cd serial/chat
./chat_serial /tmp/ttyB 115200

# Start chatting between terminals
```

### Advanced socat Testing
```bash
# Create multiple virtual ports with fixed paths
socat -d -d pty,raw,echo=0,link=/tmp/ttyA1 pty,raw,echo=0,link=/tmp/ttyB1 &
socat -d -d pty,raw,echo=0,link=/tmp/ttyA2 pty,raw,echo=0,link=/tmp/ttyB2 &

# Test with different baud rates
socat -d -d pty,raw,echo=0,link=/tmp/ttyA,ispeed=9600,ospeed=9600 pty,raw,echo=0,link=/tmp/ttyB,ispeed=9600,ospeed=9600
```

## Prerequisites

- Serial port device connected (or socat for virtual testing)
- Appropriate permissions to access serial port
- Matching baud rate between device and application

### Installing socat for Virtual Testing

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install socat
```

#### CentOS/RHEL/Fedora
```bash
sudo yum install socat
# or
sudo dnf install socat
```

#### macOS
```bash
brew install socat
```

#### Windows
```bash
# Via WSL (Windows Subsystem for Linux)
wsl --install
# Then in WSL:
sudo apt install socat

# Or via Cygwin
# Download and install Cygwin, then install socat package
```

## Troubleshooting

### Permission Denied
```bash
# Add user to dialout group (Linux)
sudo usermod -a -G dialout $USER
# Log out and log back in
```

### Device Not Found
- Check if device is connected: `ls /dev/tty*`
- Verify device path: `/dev/ttyUSB0`, `/dev/ttyACM0`, etc.
- On Windows: Check Device Manager for COM ports

### Connection Issues
- Ensure baud rate matches device settings
- Check if another application is using the port
- Verify cable and device functionality

## Platform-Specific Notes

### Linux
- Common devices: `/dev/ttyUSB0`, `/dev/ttyACM0`
- May need udev rules for consistent device naming

### Windows
- Use COM ports: `COM3`, `COM4`, etc.
- Check Device Manager for available ports

### macOS
- Use `/dev/tty.usbserial-*` or `/dev/tty.usbmodem*`
- May need to install drivers for some devices
