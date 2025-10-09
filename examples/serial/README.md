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

## Prerequisites

- Serial port device connected
- Appropriate permissions to access serial port
- Matching baud rate between device and application

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
