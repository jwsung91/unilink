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
