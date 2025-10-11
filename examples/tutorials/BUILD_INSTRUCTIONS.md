# Building Tutorial Examples

Quick guide to compile and run tutorial examples.

## Quick Build (Recommended)

From project root:

\`\`\`bash
# Configure (first time only)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build tutorial examples
cmake --build build --target tutorial_simple_client
cmake --build build --target tutorial_my_first_client
cmake --build build --target tutorial_echo_server
cmake --build build --target tutorial_chat_server

# Or build all at once
cmake --build build -j
\`\`\`

## Run Examples

\`\`\`bash
# Simple client (needs server on port 8080)
./build/examples/tutorials/tutorial_simple_client

# My first client
./build/examples/tutorials/tutorial_my_first_client [host] [port]

# Echo server
./build/examples/tutorials/tutorial_echo_server [port]

# Chat server
./build/examples/tutorials/tutorial_chat_server [port]
\`\`\`

## Manual Compilation

If CMake is not available:

\`\`\`bash
g++ -std=c++17 getting_started/simple_client.cpp \\
    -I../../unilink \\
    -L../../build \\
    -lunilink -lboost_system -pthread \\
    -o simple_client
\`\`\`

See [README.md](README.md) for detailed instructions.
