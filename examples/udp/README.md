# UDP Sender and Receiver

Minimal UDP examples using the current builder API from `unilink/unilink.hpp`.

These programs are intentionally small. They do not parse command-line options.

## Receiver Usage

```bash
./udp_receiver
```

The receiver binds to local port `9000` and prints incoming datagrams.

## Sender Usage

```bash
./udp_sender
```

The sender uses an ephemeral local port and sends user-entered messages to `127.0.0.1:9000`.

## Quick Start (Two Terminals)

**Terminal 1: Receiver**

```bash
cd examples/udp
./udp_receiver
```

**Terminal 2: Sender**

```bash
cd examples/udp
./udp_sender
```

Type lines into the sender terminal. Each line is sent immediately, and the receiver prints it.

## Notes

- `udp_receiver.cpp` is the simplest "bind and print" example.
- `udp_sender.cpp` is the simplest "set remote endpoint and send" example.
- If you need argument parsing or richer UDP workflows, use these files as a starting point rather than expecting a CLI tool.
