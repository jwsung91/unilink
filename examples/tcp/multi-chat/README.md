# Multi-Client TCP Chat Server and Client

Multi-client TCP chat examples using the current public API.

## Server Usage

```bash
./multi_chat_tcp_server <port>
```

Examples:

```bash
# Default port is 8080 when omitted
./multi_chat_tcp_server

# Custom port
./multi_chat_tcp_server 9000
```

## Client Usage

```bash
./multi_chat_tcp_client <host> <port>
```

Examples:

```bash
# Default target is 127.0.0.1:8080 when omitted
./multi_chat_tcp_client

# Explicit local connection
./multi_chat_tcp_client 127.0.0.1 9000

# Remote connection
./multi_chat_tcp_client 192.168.1.100 9000
```

## Behavior

- The server accepts multiple clients with `.unlimited_clients()`.
- Join, leave, and chat messages are rebroadcast to all clients.
- The server operator can send `[Admin]: ...` messages from stdin.
- Clients send stdin lines and print every broadcast they receive.

## Expected Output

Server:

```text
[INFO] [server] [main] Multi-Chat Server started on port 8080
[INFO] [server] [STATE] Client 1 joined (IP: 127.0.0.1:...)
[INFO] [server] [CHAT] [Client 1]: Hello everyone!
```

Client:

```text
*** Connected to Multi-Chat Server ***
[Client 1]: Hello everyone!
```

## Quick Test

```bash
# Terminal 1
./multi_chat_tcp_server

# Terminal 2
./multi_chat_tcp_client

# Terminal 3
./multi_chat_tcp_client
```

## Notes

- Clients receive the server's rebroadcast stream, including their own messages.
- This is a lightweight sample, not a production chat service.
- Both binaries use the public `unilink/unilink.hpp` API.

## Troubleshooting

### Client Connection Issues

- Verify the server is running first.
- Check firewall settings.
- Confirm the host and port are correct.

### Message Not Received

- Verify the client is connected.
- Check whether the server is still running.
- Confirm another client or the server operator has sent data.
