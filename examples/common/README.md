# Common Functionality Examples

Small examples for logger configuration and error callback handling.

## Examples

- **logging_example**: Configures the logger and emits example log messages from builder callbacks
- **error_handling_example**: Triggers expected startup/connection failures and prints `ErrorContext`

## Logging Example

### Usage
```bash
./logging_example
```

### What it demonstrates
- Setting the logger level to `DEBUG`
- Enabling console output
- Emitting log messages with `UNILINK_LOG_INFO(...)`
- Attaching logging hooks to TCP and Serial builders

### Expected Output
```
[INFO] [main] [setup] Starting logging example...
[INFO] [main] [cleanup] Example finished.
```

## Error Handling Example

### Usage
```bash
./error_handling_example
```

### What it demonstrates
- Using `.on_error(...)` with the current wrapper API
- Reading `ErrorContext::code()` and `ErrorContext::message()`
- Waiting on `start().get()` to observe expected failures

### Expected Output
```
--- Unilink Error Handling Example ---
Server Error Detected!
Server start failed as expected.
Starting client connection attempt...
Client Error Detected!
Client connection failed as expected.
```

## Configuration

### Logging Configuration
```cpp
// Set log level and console output
Logger::instance().set_level(LogLevel::DEBUG);
Logger::instance().set_console_output(true);
```

### Error Handling Configuration
```cpp
auto client = tcp_client("127.0.0.1", 1)
    .on_error([](const ErrorContext& ctx) {
        std::cout << ctx.message() << std::endl;
    })
    .build();
```

## Use Cases

- Use `logging_example` as a starting point for logger setup.
- Use `error_handling_example` as a starting point for `on_error(...)` callback wiring.
