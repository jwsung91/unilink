# Logging Example

Demonstrates the comprehensive logging system provided by the unilink library.

## Usage

```bash
./logging_example
```

## What it demonstrates

- **Log level configuration**: Setting different log levels (DEBUG, INFO, WARNING, ERROR, CRITICAL)
- **Multiple output destinations**: Console, file, and callback outputs
- **Async logging**: Non-blocking log processing with batch operations
- **Log rotation**: Automatic file rotation and cleanup
- **Performance monitoring**: Log processing statistics and metrics

## Expected Output

```
=== Unilink Logging System Usage Example ===

1. Logging system setup
   - Setting log level to DEBUG
   - Enabling file output: unilink_example.log
   - Enabling console output
   - Configuring async logging

2. Basic logging examples
   [DEBUG] [logging] [setup] Debug message
   [INFO] [logging] [setup] Info message
   [WARNING] [logging] [setup] Warning message
   [ERROR] [logging] [setup] Error message
   [CRITICAL] [logging] [setup] Critical message

3. Async logging demonstration
   - Enabling async logging with batch processing
   - Sending multiple log messages
   - Processing logs in batches

4. Performance test
   - Sending 1000 log messages
   - Measuring processing time
   - Calculating messages per second

5. Log rotation test
   - Testing log file rotation
   - Verifying log file creation
   - Checking log file cleanup
```

## Key Features Demonstrated

### Log Level Configuration
```cpp
// Set minimum log level
Logger::instance().set_level(LogLevel::DEBUG);

// Log messages at different levels
Logger::instance().debug("component", "operation", "Debug message");
Logger::instance().info("component", "operation", "Info message");
Logger::instance().warning("component", "operation", "Warning message");
Logger::instance().error("component", "operation", "Error message");
Logger::instance().critical("component", "operation", "Critical message");
```

### Output Configuration
```cpp
// Enable console output
Logger::instance().enable_console_output();

// Enable file output
Logger::instance().set_file_output("app.log");

// Enable callback output
Logger::instance().set_callback_output([](LogLevel level, const std::string& message) {
    // Custom callback handling
});
```

### Async Logging
```cpp
// Enable async logging
Logger::instance().enable_async_logging();

// Configure async settings
AsyncLogConfig config;
config.max_queue_size = 10000;
config.batch_size = 100;
config.flush_interval = std::chrono::milliseconds(100);
Logger::instance().configure_async_logging(config);
```

## Performance Testing

The example includes performance testing that:
- Sends a large number of log messages
- Measures processing time
- Calculates messages per second
- Demonstrates async logging efficiency

## Log File Management

### Log Rotation
- Automatic log file rotation when size limit is reached
- Configurable rotation settings
- Automatic cleanup of old log files

### File Output
- Logs are written to specified file
- Configurable file path and naming
- Automatic file creation and management

## Use Cases

- **Application logging**: Comprehensive logging for applications
- **Debugging**: Detailed logging for development and debugging
- **Monitoring**: Log-based monitoring and alerting
- **Auditing**: Log-based audit trails
- **Performance analysis**: Log-based performance monitoring

## Configuration Options

### Log Levels
- **DEBUG**: Detailed debugging information
- **INFO**: General information messages
- **WARNING**: Warning messages
- **ERROR**: Error messages
- **CRITICAL**: Critical error messages

### Output Destinations
- **Console**: Standard output/error
- **File**: Log files with rotation
- **Callback**: Custom callback functions

### Async Settings
- **Queue size**: Maximum number of queued log messages
- **Batch size**: Number of messages processed in each batch
- **Flush interval**: Time interval for flushing logs
- **Backpressure**: Handling of queue overflow

## Troubleshooting

### Log Files Not Created
- Check file permissions
- Verify directory exists
- Check disk space

### Performance Issues
- Adjust async settings
- Increase queue size
- Optimize batch processing

### Log Level Issues
- Verify log level configuration
- Check if messages meet minimum level
- Ensure proper log level usage
