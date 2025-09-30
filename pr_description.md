## Summary

This PR implements immediate and medium-term improvements to enhance the unilink C++ networking library's memory safety, configurability, and performance.

## Changes Made

### 🔧 Immediate Improvements

#### 1. Memory Safety Enhancement
- **Raw pointer → unique_ptr**: Replaced `io_context*` with `std::unique_ptr<io_context>` in TcpClient
- **Automatic memory management**: Removed manual `delete` calls, completed RAII pattern
- **Thread safety**: Improved memory management in multi-threaded scenarios

#### 2. Magic Numbers Elimination
- **New constants file**: `unilink/common/constants.hpp`
- **Defined constants**:
  ```cpp
  constexpr size_t DEFAULT_BACKPRESSURE_THRESHOLD = 1 << 20;  // 1 MiB
  constexpr size_t DEFAULT_READ_BUFFER_SIZE = 4096;           // 4 KiB
  constexpr unsigned DEFAULT_RETRY_INTERVAL_MS = 2000;        // 2 seconds
  constexpr unsigned DEFAULT_CONNECTION_TIMEOUT_MS = 5000;    // 5 seconds
  ```
- **Applied to all Transport classes**: TcpClient, TcpServer, Serial

#### 3. Exception Handling Improvement
- **Specific exception handling**: Replaced generic `catch(...)` with specific exception types
- **Better error logging**: Added detailed error messages for debugging
- **Graceful degradation**: Improved error recovery mechanisms

### 🚀 Medium-term Improvements

#### 1. Memory Pool Implementation
- **New memory pool**: `unilink/common/memory_pool.hpp`
- **Template-based design**: Thread-safe buffer management
- **Global instance**: `GlobalMemoryPool::instance()` for easy access
- **Performance optimization**: Reduced allocation/deallocation overhead

#### 2. Configurable Backpressure Thresholds
- **Enhanced configuration classes**:
  - `TcpClientConfig`: Added backpressure, retry, timeout settings
  - `TcpServerConfig`: Added backpressure, max connections settings
  - `SerialConfig`: Added backpressure, retry settings
- **Validation methods**: `is_valid()`, `validate_and_clamp()`
- **Runtime configurability**: Backpressure thresholds can be adjusted at runtime

#### 3. Extended Constants Definition
- **Min/max value limits**: Safe configuration range enforcement
- **Memory pool constants**: Pool size limitations
- **Retry/timeout constants**: Consistent default values

#### 4. Test Isolation Improvement
- **shared_ptr usage**: Enhanced memory safety in TcpServer lambda functions
- **Segmentation fault resolution**: Improved object lifecycle management
- **Stable test execution**: All tests now pass consistently

#### 5. Transport Class Enhancements
- **Configuration-based backpressure**: Removed hardcoded values
- **Constructor validation**: Automatic configuration validation
- **Consistent initialization**: Applied to all Transport classes

## Technical Details

### Memory Management
- Replaced raw pointers with smart pointers for automatic memory management
- Implemented RAII pattern throughout the codebase
- Added memory pool for high-frequency allocations

### Configuration System
- Added comprehensive validation for all configuration parameters
- Implemented clamping to ensure values stay within safe ranges
- Made backpressure thresholds configurable per transport instance

### Error Handling
- Improved exception handling with specific error types
- Added detailed error logging for better debugging
- Enhanced error recovery mechanisms

## Testing

- **All existing tests pass**: 9/9 tests successful
- **Memory safety verified**: No segmentation faults
- **Backward compatibility maintained**: No breaking changes
- **Performance improved**: Reduced allocation overhead

## Files Changed

- `unilink/common/constants.hpp` (new)
- `unilink/common/memory_pool.hpp` (new)
- `unilink/config/tcp_client_config.hpp`
- `unilink/config/tcp_server_config.hpp`
- `unilink/config/serial_config.hpp`
- `unilink/transport/tcp_client/tcp_client.hpp`
- `unilink/transport/tcp_client/tcp_client.cc`
- `unilink/transport/tcp_server/tcp_server_session.hpp`
- `unilink/transport/tcp_server/tcp_server_session.cc`
- `unilink/transport/tcp_server/tcp_server.cc`
- `unilink/transport/serial/serial.hpp`
- `unilink/transport/serial/serial.cc`

## Impact

- **Memory Safety**: Eliminated potential memory leaks and segmentation faults
- **Configurability**: Users can now fine-tune performance parameters
- **Performance**: Reduced allocation overhead with memory pooling
- **Maintainability**: Better code organization with constants and validation
- **Stability**: Improved test reliability and error handling

## Breaking Changes

None. All changes are backward compatible.

## Future Work

- [ ] Implement comprehensive error code system
- [ ] Add performance monitoring and metrics
- [ ] Enhance documentation with usage examples
- [ ] Add more configuration validation rules
