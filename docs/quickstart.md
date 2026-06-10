# Quick Start

Full quickstart and tutorials:

https://github.com/unilink-lab/unilink-docs

## Minimal CMake

Create `CMakeLists.txt` next to `main.cpp`:

```cmake
cmake_minimum_required(VERSION 3.12)
project(unilink_quickstart LANGUAGES CXX)

find_package(unilink CONFIG REQUIRED)

add_executable(unilink_quickstart main.cpp)
target_link_libraries(unilink_quickstart PRIVATE unilink::unilink)
target_compile_features(unilink_quickstart PRIVATE cxx_std_20)
```

## Minimal TCP client

This client expects a TCP server to be listening on `127.0.0.1:8080`.
For a runnable client/server pair, see the full tutorials in
`unilink-docs`.

```cpp
#include <iostream>
#include <unilink/unilink.hpp>

int main() {
    auto client = unilink::tcp_client("127.0.0.1", 8080)
        .on_data([](const unilink::MessageContext& ctx) {
            std::cout << "received " << ctx.data().size() << " bytes\n";
        })
        .on_error([](const unilink::ErrorContext& ctx) {
            std::cerr << "error: " << ctx.message() << "\n";
        })
        .build();

    if (!client->start_sync()) {
        return 1;
    }

    client->send("hello");
    client->stop();
    return 0;
}
```

## Notes

- Callbacks are optional for construction.
- `on_error(...)` is recommended for production workflows.
- `ctx.data()` is a callback-scoped view. Copy data if it must outlive the callback.
- Use `try_send(...)` for non-blocking producer loops.
- Use `RuntimeStats` for diagnostics and queue/drop visibility.

Related docs:

- [Callback data lifetime](callbacks.md)
- [Error model](error_model.md)
