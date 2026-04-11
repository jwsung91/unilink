# Building Tutorial Examples

Quick guide to compile and run tutorial examples.

## Quick Build

From the project root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target tutorial_simple_client tutorial_my_first_client tutorial_echo_server tutorial_chat_server
```

## Run Examples

```bash
# Simple client (needs a server on port 8080)
./build/bin/tutorial_simple_client

# Interactive client
./build/bin/tutorial_my_first_client [host] [port]

# Echo server (fixed to port 8080)
./build/bin/tutorial_echo_server

# Chat server (fixed to port 8080)
./build/bin/tutorial_chat_server
```

## Notes

- Tutorial binaries are currently emitted under `build/bin/`.
- `tutorial_echo_server` and `tutorial_chat_server` do not parse CLI port arguments.
- See [README.md](README.md) for example workflows.
