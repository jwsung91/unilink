## 2025-05-15 - std::function blocks move-only handlers
**Learning:** `std::function` requires the callable to be CopyConstructible. This prevents using move-only lambdas (e.g. capturing `std::unique_ptr` by move) as completion handlers if the interface uses `std::function`.
**Action:** When designing interfaces for async callbacks, favor templates or `std::move_only_function` (C++23) / custom type-erasure that supports move-only types to allow zero-allocation optimizations. In `TcpServerSession`, optimization was blocked because `TcpSocketInterface::async_write` takes `std::function`.

## 2026-01-29 - Expensive String Construction in Logging
**Learning:** String concatenation and `std::to_string` in logging macros are evaluated even when logging is disabled if the macro lacks an explicit level check. This can cost ~9μs per call.
**Action:** Always wrap logging macros with `if (level <= CURRENT_LEVEL)` to short-circuit argument evaluation.

## 2026-01-29 - Test Synchronization in Async Stop Contracts
**Learning:** In asynchronous systems, callbacks can validly fire during the `stop()` sequence initiation. Tests asserting 'no callbacks after stop' must set their verification flags *after* the `stop()` method returns (implying the stop signal has propagated), not before.
**Action:** Order test operations as `object->stop(); flag = true;` instead of `flag = true; object->stop();` to avoid race condition failures in CI.

## 2026-02-05 - MemoryPool Lock Contention and Exhaustion
**Learning:** Initializing `std::vector` with `nullptr` holes and managing `free_indices` queue caused memory pool exhaustion ("pool death") and unnecessary complexity. Allocating memory inside `lock_guard` caused high contention under load.
**Action:** Use a simple `std::vector` as a stack for free buffers. Perform `new` allocations outside the lock. Enforce strict capacity limits (`capacity_`) to prevent unbounded growth.
