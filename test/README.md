# unilink-test

This directory contains unit tests to verify the correctness and stability of the `unilink` library. The tests are written using the GoogleTest framework.

## How to Run Tests

From the project's root directory, you can build and run the tests using the following commands.

```bash
# 1. Configure the project with tests enabled
cmake -S . -B build -DBUILD_TESTING=ON

# 2. Build the targets
cmake --build build -j

# 3. Run the tests
cd build
ctest --output-on-failure
```

## About Tests

### Test Serial

The `test_serial.cc` file contains unit tests for the `unilink::transport::Serial` class. These tests leverage the Google Mock framework to verify the class's behavior in a controlled environment, independent of actual hardware.

The core testing strategy involves:

- **Dependency Injection**: The `Serial` class is designed to depend on an `ISerialPort` interface rather than directly on `boost::asio::serial_port`.
- **Mocking**: For testing, a `MockSerialPort` is created and injected into the `Serial` object. This mock object simulates the behavior of a real serial port.

This approach allows for precise unit testing of:

- **Connection Logic**: Verifying that the `open()` and `set_option()` methods on the port are called correctly and that `on_state` callbacks are triggered with the appropriate states (`Connecting`, `Connected`).
- **Data Reception**: Ensuring that when the mock port simulates receiving data, the `on_bytes` callback is correctly invoked with the right payload.
- **Data Transmission**: Confirming that a call to `async_write_copy` results in the correct data being passed to the mock port's `async_write` method.

By isolating the `Serial` class from its I/O dependencies, these tests provide a fast, stable, and platform-independent way to validate its core logic, including advanced scenarios:

- **Error Handling and Retries**: Validating that the class correctly handles connection failures (e.g., `open()` fails) and attempts to reconnect according to the configured retry policy.
- **Write Error Handling**: Checking that asynchronous write errors are properly managed, leading to the correct state transition (e.g., to an `Error` state when `reopen_on_error` is false).
- **Write Queuing**: Verifying that multiple consecutive `async_write_copy` calls are queued and executed in the correct order, ensuring data integrity.

### Test TCP Server

The `test_tcp_server.cc` file contains comprehensive unit tests for the `unilink::transport::TcpServer` and `unilink::transport::TcpServerSession` classes. These tests verify the TCP server functionality using mock objects and controlled test scenarios.

#### Key Testing Components

- **StateTracker Class**: A utility class for managing and tracking state transitions during tests, providing thread-safe state monitoring with condition variables.

- **MockTcpSocket and MockTcpAcceptor**: Mock implementations of TCP socket and acceptor interfaces using Google Mock framework to simulate network behavior without actual network operations.

- **TcpServerTest and TcpServerSessionTest Fixtures**: Test fixtures that provide common setup and teardown functionality for TCP server tests.

#### Test Categories

**Basic Functionality Tests:**
- Server creation and initialization
- Callback registration and management
- Write operations without active connections
- Configuration handling

**Advanced Operation Tests:**
- Multiple write operation queuing
- Backpressure handling
- Concurrent operations and thread safety
- Callback replacement and clearing
- Memory management and resource cleanup

**Connection Lifecycle Tests:**
- Client connection and disconnection handling
- Multiple client connection cycles
- Disconnection during data transmission
- Connection recovery scenarios
- Rapid connect/disconnect cycles

**Future and Asynchronous Operation Tests:**
- `future.wait_for` operations with various timeout values
- Promise exception handling
- Shared future operations
- Future chains and complex workflows
- Multiple concurrent future operations

**Error Handling and Edge Cases:**
- Port binding errors
- Invalid configurations
- Empty and large data handling
- Rapid state changes
- Pending operations during disconnection

#### Testing Strategy

The TCP server tests use a comprehensive approach that includes:

- **Mock-based Testing**: Uses Google Mock to simulate network operations without requiring actual network connections
- **State Verification**: Tracks and verifies state transitions throughout the server lifecycle
- **Concurrency Testing**: Validates thread safety and concurrent operation handling
- **Error Simulation**: Tests various error scenarios and recovery mechanisms
- **Performance Testing**: Validates handling of large data volumes and rapid operations

### Enhanced Serial Tests

The serial tests have been significantly enhanced with advanced testing patterns and comprehensive error scenario coverage:

#### Advanced Testing Features

**Builder Pattern for Mock Configuration:**
- `MockPortBuilder` class provides fluent interface for configuring mock serial ports
- Simplifies test setup and makes tests more readable and maintainable
- Supports various connection scenarios (successful, failed, retryable)

**Error Scenario Management:**
- `ErrorScenario` class provides factory methods for common error types
- Supports different error categories: ConnectionFailure, ReadError, WriteError, PortDisconnection, TimeoutError, PermissionDenied, DeviceBusy
- Each scenario includes retryability information and proper error code setup

**Comprehensive State Tracking:**
- Enhanced `StateTracker` class with thread-safe state monitoring
- Supports waiting for specific states or state counts
- Provides state history and verification capabilities

#### Test Categories

**Connection Tests:**
- Basic connection establishment and state callbacks
- Connection failure handling with retry mechanisms
- Retryable vs non-retryable error scenarios

**Data Transfer Tests:**
- Data reception through read callbacks
- Data transmission through write operations
- Multiple write operation queuing
- Write error handling and recovery

**Future and Asynchronous Operations:**
- `future.wait_for` operations with various timeout scenarios
- Promise exception handling and propagation
- Shared future operations with multiple waiters
- Future chains and dependent operations
- Multiple concurrent future operations

**Error Handling and Recovery:**
- Connection failure retry mechanisms
- Write error handling with different retry policies
- Read error handling and recovery
- Comprehensive error scenario testing using `ErrorScenario` class

**Advanced Scenarios:**
- Future operations in callbacks (verifying non-blocking behavior)
- Various timeout value handling
- Exception propagation in promise/future operations
- Complex future workflows and chains

#### Testing Patterns

**Given-When-Then Structure:**
- Clear test structure with setup, execution, and verification phases
- Improved test readability and maintainability

**Builder Pattern Usage:**
- Fluent interface for mock configuration
- Reduced boilerplate code in test setup
- More expressive test scenarios

**Error Scenario Testing:**
- Systematic approach to error testing
- Reusable error scenario definitions
- Comprehensive coverage of error conditions

**Concurrency Testing:**
- Thread safety validation
- Concurrent operation testing
- Race condition prevention verification
