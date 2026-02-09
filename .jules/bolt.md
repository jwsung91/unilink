## 2025-05-15 - std::function blocks move-only handlers
**Learning:** `std::function` requires the callable to be CopyConstructible. This prevents using move-only lambdas (e.g. capturing `std::unique_ptr` by move) as completion handlers if the interface uses `std::function`.
**Action:** When designing interfaces for async callbacks, favor templates or `std::move_only_function` (C++23) / custom type-erasure that supports move-only types to allow zero-allocation optimizations. In `TcpServerSession`, optimization was blocked because `TcpSocketInterface::async_write` takes `std::function`.

## 2026-01-29 - Expensive String Construction in Logging
**Learning:** String concatenation and `std::to_string` in logging macros are evaluated even when logging is disabled if the macro lacks an explicit level check. This can cost ~9μs per call.
**Action:** Always wrap logging macros with `if (level <= CURRENT_LEVEL)` to short-circuit argument evaluation.

## 2026-01-29 - Test Synchronization in Async Stop Contracts
**Learning:** In asynchronous systems, callbacks can validly fire during the `stop()` sequence initiation. Tests asserting 'no callbacks after stop' must set their verification flags *after* the `stop()` method returns (implying the stop signal has propagated), not before.
**Action:** Order test operations as `object->stop(); flag = true;` instead of `flag = true; object->stop();` to avoid race condition failures in CI.

## 2026-01-29 - Macro Safety in Conditional Initialization
**Learning:** Macros that expand to multiple statements (e.g. declaration + if check) are unsafe in single-statement contexts (like `if (cond) MACRO;`).
**Action:** Use the ternary operator for conditional initialization in macros to keep the expansion as a single statement: `auto var = (check) ? val : default;`.

## 2026-02-18 - MemoryPool Slot Leak Fix
**Learning:** Using `std::vector` + `std::queue` + `nullptr` holes for memory pooling caused unbounded growth of the vector ("slot leak") and poor cache locality.
**Action:** Replaced with a simple LIFO stack (`std::vector` back/pop_back). Ensure `capacity` is strictly enforced to prevent unbounded growth.

## 2026-10-24 - MemoryPool Vector Reallocation
**Learning:** Reserving `std::vector` capacity based on initial estimate instead of maximum limit in `MemoryPool` caused frequent reallocations (copying pointers) when the pool filled up, increasing runtime latency during heavy load.
**Action:** Reserve `capacity` (max size) in constructor to prevent reallocations during runtime. The small upfront memory cost (pointers only) is worth the stability.

## 2026-02-09 - [Hidden Overhead in Default Configuration]
**Learning:** The `UNILINK_ENABLE_MEMORY_TRACKING` option was enabled by default (even in Release builds), introducing significant mutex locking overhead on every allocation/deallocation in the memory pool.
**Action:** Always audit `CMakeLists.txt` and `options.cmake` files for debug features enabled by default in production-oriented libraries, as these can drastically impact performance without code changes.
