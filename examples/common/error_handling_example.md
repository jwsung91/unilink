# Error Handling Example

Demonstrates the current `on_error(...)` callback flow with the wrapper API.

## Usage

```bash
./error_handling_example
```

## What it demonstrates

- Registering `.on_error(...)` on builders
- Inspecting `ErrorContext::code()` and `ErrorContext::message()`
- Handling expected failures from `start().get()`

## Expected Output

```
--- Unilink Error Handling Example ---
Server Error Detected!
Server start failed as expected.
Starting client connection attempt...
Client Error Detected!
Client connection failed as expected.
```

The exact message text depends on the platform and socket error returned by the OS.

## Key Snippet

```cpp
auto client = tcp_client("127.0.0.1", 1)
    .on_error([](const ErrorContext& ctx) {
        std::cout << "Code: " << static_cast<int>(ctx.code()) << std::endl;
        std::cout << "Message: " << ctx.message() << std::endl;
    })
    .build();
```

## Use Cases

- Start from this file when you need callback-based error reporting around `start()`.
- Extend it with retry or recovery policy in your application code rather than expecting that behavior from this example.
