# Common Functionality Examples

Examples demonstrating common functionality and utilities provided by the unilink library.

## Examples

- **logging_example**: Demonstrates the logging system usage
- **error_handling_example**: Shows error handling system usage

## Logging Example

### Usage
```bash
./logging_example
```

### What it demonstrates
- Setting log levels (DEBUG, INFO, WARNING, ERROR, CRITICAL)
- Console and file output configuration
- Async logging with batch processing
- Log rotation and cleanup
- Performance monitoring

### Expected Output
```
=== Unilink Logging System Usage Example ===

1. Logging system setup
2. Basic logging examples
3. Async logging demonstration
4. Performance test
5. Log rotation test
```

## Error Handling Example

### Usage
```bash
./error_handling_example
```

### What it demonstrates
- Error handler setup and configuration
- Error callback registration
- Different error levels and types
- Error recovery mechanisms
- Error statistics and monitoring

### Expected Output
```
=== Unilink Error Handling System Usage Example ===

1. Error handler setup
2. Basic error handling
3. Error recovery demonstration
4. Error statistics
```

## Key Features

### Logging System
- **Multiple output destinations**: Console, file, callback
- **Async logging**: Non-blocking log processing
- **Log rotation**: Automatic file rotation and cleanup
- **Performance monitoring**: Log processing statistics
- **Configurable levels**: DEBUG, INFO, WARNING, ERROR, CRITICAL

### Error Handling System
- **Centralized error management**: Single point for all errors
- **Error categorization**: Different error types and levels
- **Recovery mechanisms**: Automatic retry and fallback
- **Error monitoring**: Statistics and reporting
- **Custom error callbacks**: User-defined error handling

## Configuration

### Logging Configuration
```cpp
// Set log level
Logger::instance().set_level(LogLevel::DEBUG);

// Enable file output
Logger::instance().set_file_output("app.log");

// Enable console output
Logger::instance().enable_console_output();

// Configure async logging
Logger::instance().enable_async_logging();
```

### Error Handling Configuration
```cpp
// Set minimum error level
ErrorHandler::instance().set_min_error_level(ErrorLevel::INFO);

// Register error callback
ErrorHandler::instance().register_callback([](const ErrorInfo& error) {
    // Handle error
});
```

## Use Cases

- **Application logging**: Comprehensive logging for applications
- **Error monitoring**: Centralized error handling and reporting
- **Debugging**: Detailed logging for development and debugging
- **Production monitoring**: Error tracking and performance monitoring
