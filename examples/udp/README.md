# UDP Sender and Receiver

UDP sender/receiver examples using the unilink UDP builder. The receiver listens on a local port and optionally replies to the first peer it learns. The sender pushes periodic messages and prints replies.

## Receiver Usage

```bash
./udp_receiver --local-port <port> [--local-ip <ip>] [--reply]
```

### Examples

```bash
# Listen on port 9000, reply to the first peer
./udp_receiver --local-port 9000 --reply

# Bind to a specific interface without replying
./udp_receiver --local-port 9000 --local-ip 0.0.0.0
```

## Sender Usage

```bash
./udp_sender --remote-ip <ip> --remote-port <port> [--local-port <port>] [--local-ip <ip>] \
  [--interval-ms <ms>] [--count <n>] [--message <text>]
```

### Examples

```bash
# Send "ping" every 500ms to the receiver, using local port 9001
./udp_sender --remote-ip 127.0.0.1 --remote-port 9000 --local-port 9001 --interval-ms 500 --message "ping"

# Send five messages then exit (local port defaults to remote+1)
./udp_sender --remote-ip 127.0.0.1 --remote-port 9000 --count 5
```

## Quick Start (Two Terminals)

**Terminal 1: Receiver (reply enabled)**

```bash
cd examples/udp
./udp_receiver --local-port 9000 --reply
```

**Terminal 2: Sender**

```bash
cd examples/udp
./udp_sender --remote-ip 127.0.0.1 --remote-port 9000 --local-port 9001 --message "ping" --interval-ms 500
```

You should see the receiver log inbound packets and reply once the first peer is learned. The sender logs every transmit and prints any replies.

## Notes

- The receiver only replies after the first peer is learned; writes before that are ignored by design.
- The sender's `--count` defaults to `0` (infinite). Use a positive value to stop after a fixed number of sends.
- Default local port for the sender is `remote-port + 1`, matching the documented 9000/9001 example pairing.
