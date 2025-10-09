# Error Handling Example

Demonstrates the comprehensive error handling system provided by the unilink library.

## Usage

```bash
./error_handling_example
```

## What it demonstrates

- **Error handler setup**: Configuring the error handling system
- **Error callback registration**: Custom error handling callbacks
- **Different error levels**: Various error types and severity levels
- **Error recovery mechanisms**: Automatic retry and fallback strategies
- **Error statistics**: Error monitoring and reporting

## Expected Output

```
=== Unilink Error Handling System Usage Example ===

1. Error handler setup
   - Setting minimum error level to INFO
   - Registering error callback
   - Configuring error handling

2. Basic error handling
   🚨 Error occurred: [INFO] [error_handling] [test] Test error message
   🚨 Error occurred: [WARNING] [error_handling] [test] Test warning message
   🚨 Error occurred: [ERROR] [error_handling] [test] Test error message
   🚨 Error occurred: [CRITICAL] [error_handling] [test] Test critical message

3. Error recovery demonstration
   - Simulating recoverable error
   - Attempting error recovery
   - Successfully recovered from error

4. Error statistics
   - Total errors: 4
   - Errors by level: INFO=1, WARNING=1, ERROR=1, CRITICAL=1
   - Recovery attempts: 1
   - Successful recoveries: 1
```

## Key Features Demonstrated

### Error Handler Setup
```cpp
// Set minimum error level
ErrorHandler::instance().set_min_error_level(ErrorLevel::INFO);

// Register error callback
ErrorHandler::instance().register_callback([](const ErrorInfo& error) {
    std::cout << "🚨 Error occurred: " << error.get_summary() << std::endl;
});
```

### Error Reporting
```cpp
// Report different types of errors
ErrorHandler::instance().report_error(ErrorLevel::INFO, "component", "operation", "Info message");
ErrorHandler::instance().report_error(ErrorLevel::WARNING, "component", "operation", "Warning message");
ErrorHandler::instance().report_error(ErrorLevel::ERROR, "component", "operation", "Error message");
ErrorHandler::instance().report_error(ErrorLevel::CRITICAL, "component", "operation", "Critical message");
```

### Error Recovery
```cpp
// Simulate recoverable error
try {
    // Simulate error condition
    throw std::runtime_error("Simulated error");
} catch (const std::exception& e) {
    // Report error and attempt recovery
    ErrorHandler::instance().report_error(ErrorLevel::ERROR, "component", "operation", e.what());
    
    // Attempt recovery
    if (attempt_recovery()) {
        ErrorHandler::instance().report_recovery("component", "operation", "Successfully recovered");
    }
}
```

## Error Levels

### INFO
- General information messages
- Non-critical issues
- Status updates

### WARNING
- Potential issues
- Non-fatal errors
- Attention required

### ERROR
- Serious errors
- Operation failures
- System issues

### CRITICAL
- Critical system errors
- Fatal failures
- Immediate attention required

## Error Recovery Mechanisms

### Automatic Retry
- Configurable retry attempts
- Exponential backoff
- Retry limits

### Fallback Strategies
- Alternative implementations
- Graceful degradation
- Service substitution

### Error Isolation
- Error containment
- System protection
- Fault tolerance

## Error Statistics

The example demonstrates error statistics including:
- **Total error count**: Number of errors reported
- **Errors by level**: Breakdown by error severity
- **Recovery attempts**: Number of recovery attempts
- **Successful recoveries**: Number of successful recoveries
- **Error rates**: Error frequency and patterns

## Use Cases

### Application Error Handling
- Centralized error management
- Consistent error reporting
- Error monitoring and alerting

### System Monitoring
- Error tracking and analysis
- Performance monitoring
- System health checks

### Debugging and Development
- Error logging and tracing
- Debug information
- Development support

## Configuration Options

### Error Levels
- **Minimum level**: Only errors above this level are processed
- **Level filtering**: Filter errors by severity
- **Level-specific handling**: Different handling for different levels

### Callback Configuration
- **Custom callbacks**: User-defined error handling
- **Multiple callbacks**: Support for multiple error handlers
- **Callback priority**: Priority-based callback execution

### Recovery Settings
- **Retry attempts**: Number of retry attempts
- **Retry intervals**: Time between retry attempts
- **Recovery strategies**: Different recovery approaches

## Troubleshooting

### Error Callbacks Not Triggered
- Check error level configuration
- Verify callback registration
- Ensure errors meet minimum level

### Recovery Issues
- Check recovery implementation
- Verify error conditions
- Ensure proper error handling

### Performance Issues
- Monitor error processing overhead
- Optimize error handling code
- Consider async error processing
