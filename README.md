# unilink

## One interface, reliable connections

`unilink` is a C++ library that provides a unified, high-level Builder API for TCP (client/server) and Serial ports. It simplifies network and serial programming with a fluent, easy-to-use interface that handles all the complexity behind the scenes.

---

## Features

- **Builder API**: Fluent, chainable interface for creating and configuring communication channels
- **Unified Interface**: Single API for TCP (Client/Server) and Serial communication
- **Asynchronous Operations**: Callback-based, non-blocking I/O for high performance
- **Automatic Reconnection**: Built-in, configurable reconnection logic for clients and serial ports
- **Thread-Safe**: Managed I/O thread and thread-safe operations with advanced concurrency primitives
- **Memory Safety**: Comprehensive memory safety features including bounds checking, leak detection, and safe data handling
- **Type Safety**: Safe type conversions and data buffer management with compile-time and runtime checks
- **Auto Management**: Optional automatic resource management and lifecycle control
- **Modular Design**: Optional configuration management API for advanced users
- **Optimized Builds**: Configurable compilation for minimal footprint
- **Debugging Support**: Built-in memory tracking and AddressSanitizer support for development

---

## Quick Start

`unilink` provides a simple Builder API for creating communication channels. The library handles all the complexity of network and serial communication behind the scenes, providing a clean and intuitive interface for developers.

### Basic Usage

- **TCP Client**: Connect to remote servers with automatic reconnection
- **TCP Server**: Accept multiple client connections with thread-safe operations  
- **Serial Communication**: Interface with serial devices and embedded systems
- **Automatic Management**: Optional automatic resource management and lifecycle control

---

## Usage

### Basic Usage (Builder API)

Most users will only need the Builder API for simple communication:

- **TCP Client**: Create client connections with automatic reconnection
- **TCP Server**: Set up servers to accept multiple client connections
- **Serial Communication**: Interface with serial devices and embedded systems
- **Event Handlers**: Set up callbacks for connection, data, and error events
- **Auto Management**: Enable automatic resource management and lifecycle control

### Advanced Usage (Configuration Management API)

For advanced users who need dynamic configuration:

- **Dynamic Configuration**: Runtime configuration management
- **File-based Settings**: Save and load configurations from files
- **Flexible Parameters**: Configure connection settings, retry intervals, and device parameters
- **Environment Adaptation**: Adjust settings based on deployment environment

### Build Configuration

Choose the build configuration that fits your needs:

- **Minimal Build** (`UNILINK_ENABLE_CONFIG=OFF`): Only Builder API, smaller footprint
- **Full Build** (`UNILINK_ENABLE_CONFIG=ON`): Includes configuration management API

---

## Project Structure

```bash
.
├── unilink/                    # Core library source and headers
│   ├── common/                 # Common utilities and safety features
│   │   ├── safe_data_buffer.*  # Type-safe data buffer implementation
│   │   ├── safe_span.hpp       # C++17 compatible span-like class
│   │   ├── thread_safe_state.* # Thread-safe state management
│   │   ├── memory_tracker.*    # Memory tracking and leak detection
│   │   └── memory_pool.*       # Advanced memory pool implementation
│   ├── transport/              # Transport layer implementations
│   ├── wrapper/                # High-level wrapper APIs
│   ├── builder/                # Builder pattern implementations
│   └── config/                 # Configuration management
├── examples/                   # Example applications
├── test/                       # Comprehensive test suite (132 tests)
│   ├── test_memory_safety.cc   # Memory safety tests
│   ├── test_concurrency_safety.cc # Thread safety tests
│   └── test_safe_data_buffer.cc # Type safety tests
├── CMakeLists.txt              # Top-level build script
└── README.md                   # This file
```

---

## Requirements

### System Requirements
- **Ubuntu 22.04 LTS or later** (recommended)
- **C++17 compatible compiler** (GCC 11+ or Clang 14+)
- **CMake 3.10 or later**

### Dependencies

```bash
# For the core library
sudo apt update && sudo apt install -y \
  build-essential cmake libboost-dev libboost-system-dev
```

### Ubuntu 20.04 Support
Ubuntu 20.04 support has been temporarily removed from CI/CD due to performance issues. 
If you need to build on Ubuntu 20.04, see the [Manual Build Guide](#ubuntu-2004-manual-build) below.

---

## How to Build

You can build the library with different configurations to optimize for your use case.

### Basic Build (Builder API only - recommended for most users)

```bash
# 1. Configure for minimal footprint (Builder API only)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUNILINK_ENABLE_CONFIG=OFF

# 2. Build the targets
cmake --build build -j
```

### Full Build (includes Configuration Management API)

```bash
# 1. Configure with all features
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUNILINK_ENABLE_CONFIG=ON

# 2. Build the targets
cmake --build build -j
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `UNILINK_ENABLE_CONFIG` | `ON` | Enable configuration management API |
| `UNILINK_ENABLE_MEMORY_TRACKING` | `ON` | Enable memory tracking for debugging |
| `UNILINK_ENABLE_SANITIZERS` | `OFF` | Enable AddressSanitizer and other sanitizers |
| `BUILD_EXAMPLES` | `ON` | Build example applications |
| `BUILD_TESTING` | `ON` | Build unit tests |

### Examples

```bash
# Build library and examples (minimal)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUNILINK_ENABLE_CONFIG=OFF -DBUILD_EXAMPLES=ON

# Build library and tests (full features)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUNILINK_ENABLE_CONFIG=ON -DBUILD_TESTING=ON

# Build with memory tracking enabled (for debugging)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DUNILINK_ENABLE_MEMORY_TRACKING=ON -DBUILD_TESTING=ON

# Build with AddressSanitizer (for development)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DUNILINK_ENABLE_SANITIZERS=ON -DBUILD_TESTING=ON

# Build everything
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUNILINK_ENABLE_CONFIG=ON -DBUILD_EXAMPLES=ON -DBUILD_TESTING=ON
```

---

## Performance Optimization

### Minimal Build Benefits

When building with `UNILINK_ENABLE_CONFIG=OFF`:

- **Faster Compilation**: Excludes configuration management code
- **Smaller Binary**: Reduced library size
- **Lower Memory Usage**: No configuration overhead
- **Simpler Dependencies**: Fewer include files

### When to Use Each Build

| Use Case | Recommended Build | Reason |
|----------|------------------|---------|
| Simple TCP/Serial apps | `UNILINK_ENABLE_CONFIG=OFF` | Minimal footprint, faster compilation |
| Embedded systems | `UNILINK_ENABLE_CONFIG=OFF` | Memory constraints |
| Configuration-heavy apps | `UNILINK_ENABLE_CONFIG=ON` | Dynamic configuration needed |
| Testing/Development | `UNILINK_ENABLE_CONFIG=ON` | Full feature set for testing |

---

## Memory Safety Features

`unilink` provides comprehensive memory safety features to ensure robust and secure applications:

### Safe Data Handling

The library provides type-safe data buffer management with automatic bounds checking:

- **SafeDataBuffer**: Type-safe data buffer with runtime bounds checking
- **Safe Access**: All buffer access is automatically validated
- **Safe Conversions**: Type-safe conversion utilities prevent undefined behavior
- **Memory Validation**: Comprehensive validation of data integrity

### Thread-Safe State Management

Advanced concurrency primitives for multi-threaded applications:

- **ThreadSafeState**: Read-write lock based state management
- **AtomicState**: Lock-free atomic state operations
- **ThreadSafeCounter**: Thread-safe counter operations
- **ThreadSafeFlag**: Condition variable supported flags

### Memory Tracking (Development)

Built-in memory tracking for debugging and development:

- **Allocation Tracking**: Monitor memory allocations and deallocations
- **Leak Detection**: Identify potential memory leaks
- **Performance Monitoring**: Track memory usage patterns
- **Debug Reports**: Detailed memory usage reports

### AddressSanitizer Support

Build with AddressSanitizer for advanced memory error detection:

```bash
# Build with AddressSanitizer enabled
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DUNILINK_ENABLE_SANITIZERS=ON
cmake --build build -j

# Run tests with memory error detection
cd build && ctest --output-on-failure
```

### Memory Safety Benefits

- **Bounds Checking**: All buffer access is automatically bounds-checked
- **Type Safety**: Safe type conversions prevent undefined behavior
- **Leak Detection**: Built-in memory tracking helps identify leaks
- **Thread Safety**: All state management is thread-safe by default
- **Error Prevention**: Comprehensive validation prevents common memory errors

---

## Performance Optimization

### Minimal Build Benefits

When building with `UNILINK_ENABLE_CONFIG=OFF`:

- **Faster Compilation**: Excludes configuration management code
- **Smaller Binary**: Reduced library size
- **Lower Memory Usage**: No configuration overhead
- **Simpler Dependencies**: Fewer include files

### When to Use Each Build

| Use Case | Recommended Build | Reason |
|----------|------------------|---------|
| Simple TCP/Serial apps | `UNILINK_ENABLE_CONFIG=OFF` | Minimal footprint, faster compilation |
| Embedded systems | `UNILINK_ENABLE_CONFIG=OFF` | Memory constraints |
| Configuration-heavy apps | `UNILINK_ENABLE_CONFIG=ON` | Dynamic configuration needed |
| Testing/Development | `UNILINK_ENABLE_CONFIG=ON` | Full feature set for testing |

---

## Testing

`unilink` includes comprehensive test coverage with 132 test cases covering all functionality:

### Running Tests

**Build with tests enabled:**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
```

**Run all tests:**
```bash
cmake --build build -j
ctest --test-dir build --output-on-failure
```

**Run specific test categories:**
- `./build/test/run_core_tests` - Basic functionality tests
- `./build/test/run_integration_tests` - Integration tests  
- `./build/test/run_memory_safety_tests` - Memory safety tests
- `./build/test/run_concurrency_safety_tests` - Thread safety tests
- `./build/test/run_performance_tests` - Performance benchmarks
- `./build/test/run_stress_tests` - Stress and stability tests

### Test Results

**Expected output for successful test run:**
```
Test project /path/to/unilink/build
        Start   1: BaseTest.CommonFunctionality
  1/132 Test   #1: BaseTest.CommonFunctionality ...................................   Passed    0.00 sec
        Start   2: BaseTest.ConfigManager
  2/132 Test   #2: BaseTest.ConfigManager .........................................   Passed    0.00 sec
        ...
        Start 132: MemorySafetyTest.MemoryStressTest
132/132 Test #132: MemorySafetyTest.MemoryStressTest ..............................   Passed    0.00 sec

100% tests passed, 0 tests failed out of 132

Total Test time (real) =  53.48 sec
```

**Memory Safety Test Results:**
```
[==========] Running 10 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 10 tests from MemorySafetyTest
[ RUN      ] MemorySafetyTest.MemoryTrackingBasicFunctionality
[       OK ] MemorySafetyTest.MemoryTrackingBasicFunctionality (0 ms)
[ RUN      ] MemorySafetyTest.MemoryLeakDetection
[       OK ] MemorySafetyTest.MemoryLeakDetection (0 ms)
...
[----------] 10 tests from MemorySafetyTest (1 ms total)
[----------] Global test environment tear-down
[==========] 10 tests from 1 test suite ran. (1 ms total)
[  PASSED  ] 10 tests.
```

### Test Coverage

| Category | Tests | Description |
|----------|-------|-------------|
| **Core Tests** | 22 | Basic functionality and API tests |
| **Integration Tests** | 19 | End-to-end communication tests |
| **Memory Safety Tests** | 10 | Memory tracking, leak detection, bounds checking |
| **Concurrency Safety Tests** | 9 | Thread safety and concurrent access tests |
| **Performance Tests** | 8 | Performance benchmarks and optimization tests |
| **Boundary Tests** | 8 | Edge cases and boundary condition tests |
| **Error Recovery Tests** | 9 | Error handling and recovery mechanism tests |
| **Stress Tests** | 6 | High-load and stability tests |
| **Configuration Tests** | 16 | Configuration validation and management tests |
| **API Safety Tests** | 10 | Input validation and safety tests |
| **Advanced Optimization Tests** | 18 | Advanced performance and optimization tests |
| **SafeDataBuffer Tests** | 11 | Type-safe data handling tests |

### Memory Safety Validation

The library includes extensive memory safety validation:

- **Memory Leak Detection**: Automatic tracking of allocations and deallocations
- **Bounds Checking**: All buffer access is validated at runtime
- **Thread Safety**: Comprehensive testing of concurrent access patterns
- **Type Safety**: Validation of safe type conversions and data handling
- **Stress Testing**: High-load scenarios to ensure stability under pressure

### Continuous Integration

All tests pass with 100% success rate across different build configurations:
- Debug builds with memory tracking
- Release builds with optimizations
- Sanitizer builds with AddressSanitizer
- Different compiler versions (GCC, Clang)

---

## Ubuntu 20.04 Manual Build

If you need to build on Ubuntu 20.04, follow these steps:

### Prerequisites

```bash
# Install dependencies
sudo apt update && sudo apt install -y \
  build-essential \
  cmake \
  libboost-dev \
  libboost-system-dev \
  doxygen \
  graphviz

# Install specific compiler versions
sudo apt install -y gcc-9 g++-9 clang-12
```

### Build Steps

```bash
# 1. Set compiler environment
export CC=gcc-9
export CXX=g++-9

# 2. Configure CMake
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=17 \
  -DUNILINK_ENABLE_CONFIG=ON \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_TESTING=ON

# 3. Build
cmake --build build -j $(nproc)

# 4. Run tests (optional)
cd build && ctest --output-on-failure
```

### Notes
- Ubuntu 20.04 LTS reaches end-of-life in April 2025
- Consider upgrading to Ubuntu 22.04 LTS for better long-term support
- If you encounter issues, please report them in the GitHub issues

---
