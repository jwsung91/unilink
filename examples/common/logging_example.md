# Logging Example

Demonstrates the current logger API at a minimal, runnable level.

## Usage

```bash
./logging_example
```

## What it demonstrates

- Setting the log level to `DEBUG`
- Enabling console output
- Emitting messages with `UNILINK_LOG_INFO(...)`
- Attaching logging callbacks to example builders

## Expected Output

```
[INFO] [main] [setup] Starting logging example...
[INFO] [main] [cleanup] Example finished.
```

The exact output may vary if transport creation triggers additional callbacks on your platform.

## Key Snippet

```cpp
Logger::instance().set_level(LogLevel::DEBUG);
Logger::instance().set_console_output(true);
UNILINK_LOG_INFO("main", "setup", "Starting logging example...");
```

## Use Cases

- Start from this file when you need basic logger initialization.
- Extend it if you want file output, rotation, or more complex logging policy.
