## 2025-05-15 - std::function blocks move-only handlers
**Learning:** `std::function` requires the callable to be CopyConstructible. This prevents using move-only lambdas (e.g. capturing `std::unique_ptr` by move) as completion handlers if the interface uses `std::function`.
**Action:** When designing interfaces for async callbacks, favor templates or `std::move_only_function` (C++23) / custom type-erasure that supports move-only types to allow zero-allocation optimizations. In `TcpServerSession`, optimization was blocked because `TcpSocketInterface::async_write` takes `std::function`.
