# Memory Safety Architecture

`unilink` provides comprehensive memory safety features to ensure robust and secure applications. This document describes the memory safety architecture, features, and best practices.

---

## Table of Contents

1. [Overview](#overview)
2. [Safe Data Handling](#safe-data-handling)
3. [Thread-Safe State Management](#thread-safe-state-management)
4. [Memory Tracking](#memory-tracking)
5. [AddressSanitizer Support](#addresssanitizer-support)
6. [Best Practices](#best-practices)

---

## Overview

### Memory Safety Guarantees

| Feature             | Description                      | Performance Impact                |
| ------------------- | -------------------------------- | --------------------------------- |
| **Bounds Checking** | All buffer access validated      | Minimal (<1%)                     |
| **Type Safety**     | Safe conversions prevent UB      | Zero (compile-time)               |
| **Leak Detection**  | Track allocations/deallocations  | Zero (Release builds)             |
| **Thread Safety**   | All state management thread-safe | Minimal (~2-5%)                   |
| **Memory Pools**    | Reduce fragmentation             | Positive (+30% for small buffers) |

---

### Safety Levels

`unilink` provides multiple levels of memory safety:

```mermaid
flowchart TD
    L1[Level 1: Compile-Time Safety<br/>- Type safety<br/>- RAII resource management] --> L2
    L2[Level 2: Runtime Safety (Release)<br/>- Bounds checking<br/>- Safe conversions<br/>- Thread synchronization] --> L3
    L3[Level 3: Debug Safety (Debug)<br/>- Memory tracking<br/>- Allocation tracking<br/>- Leak detection] --> L4
    L4[Level 4: Sanitizers (Optional)<br/>- AddressSanitizer<br/>- ThreadSanitizer<br/>- UndefinedBehaviorSanitizer]
```

---

## Safe Data Handling

### SafeDataBuffer

Immutable, type-safe buffer wrapper around existing data:

```cpp
#include "unilink/memory/memory_tracker.hpp"
#include "unilink/memory/safe_span.hpp"
#include "unilink/memory/safe_data_buffer.hpp"

using namespace unilink::common;

// Create from existing data
SafeDataBuffer from_vec(std::vector<uint8_t>{1, 2, 3});
SafeDataBuffer from_str(std::string("hello"));

// Read access
auto span = from_vec.as_span();      // Non-owning view
auto byte = from_vec.at(1);          // Bounds-checked
auto unchecked = from_vec.data()[0]; // Pointer access
```

---

### Features

#### 1. Construction Validation

- Rejects null pointers when size > 0
- Rejects buffers >100MB
- Copies incoming data into owned storage

---

#### 2. Safe Type Conversions

Utility functions prevent undefined behavior:

```cpp
#include "unilink/base/common.hpp"

using namespace unilink::common::safe_convert;

// Safe uint8_t* to string conversion
const uint8_t* data = ...;
size_t size = ...;
std::string str = uint8_to_string(data, size);  // Null-check included

// Safe string to uint8_t* conversion
std::string input = "Hello";
const uint8_t* bytes = string_to_uint8(input);
size_t len = input.size();

// Safe numeric conversions with bounds checking
int value = safe_cast<int>(long_value);  // Checks range
```

---

#### 3. Memory Validation

Comprehensive validation of data integrity:

```cpp
// Validate buffer state
if (buffer.is_valid()) {
    // Buffer is in consistent state
    process_data(buffer.data(), buffer.size());
} else {
    // Buffer corrupted or uninitialized
    handle_error();
}

// Explicit validation (throws on oversize)
buffer.validate();
```

**Mutation model:** Data is populated at construction; no in-place `write/append` helpers exist. Use `clear()/resize()/reserve()` only when you control the backing data size.

---

### Safe Span (C++17 Compatible)

Lightweight, non-owning view of contiguous data:

```cpp
#include "unilink/memory/safe_span.hpp"

using namespace unilink::common;

void process_data(safe_span<const uint8_t> data) {
    // Iteration (operator[] is unchecked)
    for (size_t i = 0; i < data.size(); i++) {
        uint8_t byte = data[i];
        process_byte(byte);
    }

    // Bounds-checked access
    auto subset = data.subspan(1, 2);  // throws on invalid ranges
}

// Usage
std::vector<uint8_t> buffer = {1, 2, 3, 4, 5};
process_data(safe_span<const uint8_t>(buffer));
```

**Features:**

- No ownership (lightweight)
- `at()/subspan()` are checked; `operator[]` is unchecked
- C++17 compatible (pre-C++20 `std::span`)
- Zero overhead in release builds

---

## Thread-Safe State Management

### ThreadSafeState

Read-write lock based state management:

```cpp
#include "unilink/concurrency/thread_safe_state.hpp"

using namespace unilink::common;

enum class ConnectionState {
    Closed,
    Connecting,
    Connected,
    Error
};

ThreadSafeState<ConnectionState> state(ConnectionState::Closed);

// Thread 1: Write
state.set_state(ConnectionState::Connecting);

// Thread 2: Read (concurrent safe)
ConnectionState current = state.get_state();

// Thread 3: Conditional update
bool updated = state.compare_and_set(
    ConnectionState::Connecting,  // Expected
    ConnectionState::Connected     // New value
);
```

---

### AtomicState

Lock-free atomic state operations:

```cpp
#include "unilink/concurrency/thread_safe_state.hpp"

AtomicState<int> counter(0);

// Atomic increment (thread-safe)
counter.fetch_add(1);

// Atomic compare-and-swap
int expected = 10;
bool success = counter.compare_exchange_strong(expected, 20);
```

**Use when:**

- High contention scenarios
- Low latency required
- Simple atomic types (int, bool, etc.)

---

### ThreadSafeCounter

Thread-safe counter with atomic operations:

```cpp
ThreadSafeCounter counter;

// Thread 1
counter.increment();

// Thread 2
counter.decrement();

// Thread 3
size_t value = counter.get();
```

---

### ThreadSafeFlag

Condition variable supported flags:

```cpp
ThreadSafeFlag ready_flag;

// Thread 1: Wait for flag
ready_flag.wait();  // Blocks until set
std::cout << "Ready!" << std::endl;

// Thread 2: Set flag
std::this_thread::sleep_for(std::chrono::seconds(1));
ready_flag.set();  // Unblocks waiting thread

// Thread 3: Check without blocking
if (ready_flag.is_set()) {
    // Flag is set
}
```

---

### Thread Safety Summary

| Primitive             | Lock-Free | Blocking | Use Case                 |
| --------------------- | --------- | -------- | ------------------------ |
| **ThreadSafeState**   | No        | No       | Complex state management |
| **AtomicState**       | Yes       | No       | Simple atomic types      |
| **ThreadSafeCounter** | Yes       | No       | Counters, statistics     |
| **ThreadSafeFlag**    | No        | Yes      | Synchronization, signals |

---

## Memory Tracking

### Built-in Memory Tracking

Enable for debugging and development:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DUNILINK_ENABLE_MEMORY_TRACKING=ON
```

---

### Features

#### 1. Allocation Tracking

Monitor all memory allocations and deallocations:

```cpp
#include "unilink/memory/memory_tracker.hpp"

using namespace unilink::common;

// Tracking happens automatically
auto* data = new uint8_t[1024];  // Tracked
delete[] data;  // Tracked

// Query statistics
MemoryTracker::Stats stats = MemoryTracker::instance().get_stats();
std::cout << "Total allocations: " << stats.total_allocations << std::endl;
std::cout << "Total deallocations: " << stats.total_deallocations << std::endl;
std::cout << "Current usage: " << stats.current_usage << " bytes" << std::endl;
```

---

#### 2. Leak Detection

Identify potential memory leaks:

```cpp
// At program exit, check for leaks
MemoryTracker::Stats stats = MemoryTracker::instance().get_stats();

if (stats.total_allocations != stats.total_deallocations) {
    size_t leaked = stats.total_allocations - stats.total_deallocations;
    std::cerr << "⚠️ Memory leak detected: "
              << leaked << " allocations not freed" << std::endl;

    // Get detailed report
    MemoryTracker::instance().print_report();
}
```

**Output example:**

```
=== Memory Tracking Report ===
Total allocations: 1250
Total deallocations: 1248
Leaked allocations: 2
Current memory usage: 2048 bytes
Peak memory usage: 4096 bytes
```

---

#### 3. Performance Monitoring

Track memory usage patterns:

```cpp
// Track peak memory usage
size_t peak = MemoryTracker::instance().get_peak_usage();
std::cout << "Peak memory: " << peak << " bytes" << std::endl;

// Track allocation count
size_t alloc_count = MemoryTracker::instance().get_allocation_count();
std::cout << "Allocations: " << alloc_count << std::endl;
```

---

#### 4. Debug Reports

Detailed memory usage reports:

```cpp
// Print detailed report to stdout
MemoryTracker::instance().print_report();

// Or get as string
std::string report = MemoryTracker::instance().get_report_string();
```

---

### Zero Overhead in Release

Memory tracking has **zero overhead** in Release builds:

```cpp
// In Release builds (UNILINK_ENABLE_MEMORY_TRACKING=OFF):
// All tracking calls are compiled out
// No runtime overhead
// No memory overhead
```

---

## AddressSanitizer Support

### Enable AddressSanitizer

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DUNILINK_ENABLE_SANITIZERS=ON

cmake --build build -j
```

---

### What ASan Detects

AddressSanitizer detects:

- ✅ **Use-after-free**
- ✅ **Heap buffer overflow**
- ✅ **Stack buffer overflow**
- ✅ **Use-after-return**
- ✅ **Memory leaks**
- ✅ **Double-free**
- ✅ **Invalid free**

---

### Running with ASan

```bash
# Run application
./my_app

# Run tests
cd build
ctest --output-on-failure
```

**ASan output example:**

```
=================================================================
==12345==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x602000000050
WRITE of size 4 at 0x602000000050 thread T0
    #0 0x7f8a4b5c6d42 in main
    #1 0x7f8a4b3c7082 in __libc_start_main
```

---

### Performance Impact

- **Runtime overhead:** ~2-3x slowdown
- **Memory overhead:** ~2x memory usage
- **Only for testing** - not for production

---

## Best Practices

### 1. Buffer Management

#### ✅ DO

```cpp
// Use SafeDataBuffer to wrap existing data
std::vector<uint8_t> raw(data, data + len);
SafeDataBuffer buffer(raw);
process_buffer(buffer);

// Use safe_span for views
void process(safe_span<const uint8_t> data) {
    // ...
}
```

#### ❌ DON'T

```cpp
// Avoid raw pointers without bounds checking
uint8_t* buf = new uint8_t[size];
memcpy(buf, data, len);  // No bounds check!

// Avoid pointer arithmetic without validation
uint8_t* ptr = buffer + offset;  // May go out of bounds
```

---

### 2. Type Conversions

#### ✅ DO

```cpp
// Use safe conversion utilities
std::string str = safe_convert::uint8_to_string(data, size);
const uint8_t* bytes = safe_convert::string_to_uint8(str);
```

#### ❌ DON'T

```cpp
// Avoid unsafe casts
std::string str = reinterpret_cast<const char*>(data);  // May not be null-terminated
uint8_t* bytes = const_cast<uint8_t*>(str.data());  // Removes const incorrectly
```

---

### 3. Thread Safety

#### ✅ DO

```cpp
// Use thread-safe primitives
ThreadSafeState<State> state;
ThreadSafeCounter counter;

// Let unilink handle synchronization
client->send(data);  // Already thread-safe
```

#### ❌ DON'T

```cpp
// Avoid manual locking of unilink internals
// std::lock_guard<std::mutex> lock(internal_mutex);  // DON'T DO THIS

// Avoid shared state without synchronization
// state = NewState;  // Race condition!
```

---

### 4. Memory Tracking

#### ✅ DO

```cpp
// Enable in Debug builds
cmake -DCMAKE_BUILD_TYPE=Debug -DUNILINK_ENABLE_MEMORY_TRACKING=ON

// Check for leaks at exit
auto stats = MemoryTracker::instance().get_stats();
assert(stats.total_allocations == stats.total_deallocations);
```

#### ❌ DON'T

```cpp
// Don't enable in Release builds (performance)
// cmake -DCMAKE_BUILD_TYPE=Release -DUNILINK_ENABLE_MEMORY_TRACKING=ON

// Don't ignore leak reports

```

---

### 5. Sanitizers

#### ✅ DO

```cpp
// Use during development and testing
cmake -DCMAKE_BUILD_TYPE=Debug -DUNILINK_ENABLE_SANITIZERS=ON

// Run full test suite with sanitizers
ctest --output-on-failure

// Fix all reported issues
```

#### ❌ DON'T

```cpp
// Never deploy with sanitizers enabled
// (2-3x performance penalty)

// Don't suppress sanitizer warnings without investigation
```

---

## Memory Safety Benefits

### Prevents Common Vulnerabilities

| Vulnerability       | Traditional C++ | unilink                    |
| ------------------- | --------------- | -------------------------- |
| **Buffer Overflow** | Possible        | Prevented (bounds checked) |
| **Use-After-Free**  | Possible        | Detected (ASan)            |
| **Memory Leak**     | Possible        | Detected (tracking)        |
| **Data Race**       | Possible        | Prevented (thread-safe)    |
| **Type Confusion**  | Possible        | Prevented (type-safe)      |

---

### Performance

| Feature              | Debug Overhead | Release Overhead   |
| -------------------- | -------------- | ------------------ |
| **Bounds Checking**  | ~5%            | <1%                |
| **Memory Tracking**  | ~10%           | 0% (disabled)      |
| **Thread Safety**    | ~5%            | ~2-5%              |
| **Memory Pools**     | 0%             | Negative (faster!) |
| **AddressSanitizer** | ~200-300%      | N/A (not used)     |

---

## Testing Memory Safety

### Unit Tests

```bash
# Run memory safety tests
./build/test/run_memory_safety_tests
```

**Tests cover:**

- Buffer bounds checking
- Memory leak detection
- Safe type conversions
- Thread safety
- Memory pool correctness

---

### Integration Tests

```bash
# Run with AddressSanitizer
cmake -S . -B build -DUNILINK_ENABLE_SANITIZERS=ON
cmake --build build
cd build && ctest
```

---

### Continuous Integration

All memory safety features are tested in CI/CD:

- ✅ Memory tracking enabled
- ✅ AddressSanitizer enabled
- ✅ ThreadSanitizer enabled (selected tests)
- ✅ Valgrind memcheck

See [Testing Guide](../guides/testing.md) for details.

---

## Next Steps

- [Runtime Behavior](runtime_behavior.md) - Threading and execution model
- [System Overview](README.md) - High-level architecture
- [Testing Guide](../guides/testing.md) - Memory safety testing
- [Best Practices](../guides/best_practices.md) - Safe coding patterns
