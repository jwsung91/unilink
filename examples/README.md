# Unilink Examples

This directory contains examples for the `unilink` library. The examples demonstrate a simple chat application where users can exchange text messages over TCP or Serial connections.

All log messages are timestamped in the format `YYYY-MM-DD HH:MM:SS.mmm [TAG] [TX/RX] ...`.

---

## Requirements (Ubuntu)

In addition to the main library requirements, `socat` is needed for the serial chat example.

```bash
sudo apt update && sudo apt install -y socat

# Grant serial port access (requires logout/login to take effect)
sudo usermod -a -G dialout $USER
```

---

## How to Run

First, build the project with examples enabled from the root directory:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON
cmake --build build -j
```

All executables will be in the `build/examples/` directory.

---

## Example 1: TCP Chat

This example demonstrates a client-server chat over TCP.

### Run the Server

```bash
./build/examples/chat_tcp_server 9000
```

### Run the Client (in a new terminal)

```bash
./build/examples/chat_tcp_client 127.0.0.1 9000
```

### How to Use

- Type a line of text in either console and press Enter to send it.
- Received messages will be displayed with an `[RX]` tag.
- Connection status changes are logged as `[server] state=...` or `[client] state=...`.

> **Note:** TCP is a stream-based protocol. The examples parse messages line by line, using `\n` as a delimiter.

---

## Example 2: Serial Chat (using virtual TTYs)

This example shows how to communicate between two serial ports. We use `socat` to create a virtual serial port pair for testing.

### 1. Create Virtual Serial Ports

Open a terminal and run `socat`. Keep this process running to act as a bridge.

```bash
socat -d -d pty,raw,echo=0,link=/tmp/ttyA pty,raw,echo=0,link=/tmp/ttyB
```

### 2. Run Two Chat Instances

Open two new terminals and run the chat program on each, pointing to one of the virtual ports.

```bash
# In Terminal 2
./build/examples/chat_serial /tmp/ttyA

# In Terminal 3
./build/examples/chat_serial /tmp/ttyB
```

### 3. How to Use

- Type a message in one terminal and press Enter. It will appear in the other terminal.
- Connection status is logged as `[serial] state=...`.

---

## Troubleshooting

- **No data received on serial chat:**

  - Ensure the `socat` process is still running.
  - Double-check that you are using the correct device paths (`/tmp/ttyA`, `/tmp/ttyB`).

- **Permission denied for serial port:**

  - Make sure you have logged out and back in after adding your user to the `dialout` group.

- **Serial device path changes (e.g., `/dev/ttyUSB0` to `/dev/ttyUSB1`):**
  - To avoid issues with changing device paths, it is recommended to use a stable symlink, such as those found in `/dev/serial/by-id/`.

---

All example applications can be terminated with **Ctrl+C**.
