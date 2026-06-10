# Error Model

`unilink` reports runtime failures through explicit return values, callbacks,
and exceptions. The expected path depends on when the failure is detected.

## Lifecycle errors

`start()` returns `std::future<bool>` and `start_sync()` returns `bool`.

- `true`: the channel or server reached its connected/listening state.
- `false`: startup failed. A registered `on_error(...)` callback receives the
  specific failure when the implementation can classify it.

`stop()` is idempotent and blocks until pending async operations are cancelled.
After `stop()` returns, no further callbacks should fire.

## Send errors

`send(...)` follows the configured `BackpressureStrategy`.

- `Reliable`: waits for queue pressure to clear or uses the transport's
  Reliable pending queue.
- `BestEffort`: may drop when the queue is full.

`try_send(...)`, `try_send_move(...)`, and `try_send_shared(...)` are always
non-blocking. They return `false` when the channel is stopped, disconnected,
backpressured, or full. Reliable `try_send*` calls do not enqueue into pending
queues.

`send_blocking(...)` waits until queue pressure is relieved, then enqueues using
the normal strategy-aware write path. It returns `false` if the channel stops
while waiting or cannot accept the write.

Use `RuntimeStats` to inspect accepted bytes, sent bytes, failed sends, drops,
queued bytes, pending bytes, and backpressure state.

## Async runtime errors

Use `on_error(...)` for production workflows. `ErrorContext` contains:

- `code()`: structured `ErrorCode`
- `message()`: diagnostic message
- `client_id()`: optional server-side client identifier

Transport callbacks may also log internal diagnostic details.

## Validation and configuration errors

Builder and config validation errors may throw `diagnostics::ValidationException`
or related `diagnostics::UnilinkException` types when invalid input is detected
synchronously.

Configuration APIs can also return validation results or boolean status where
the API is designed as a query/update operation.

## Exceptions

Public-facing exceptions should prefer the `diagnostics::UnilinkException`
hierarchy. Some lower-level utility APIs intentionally preserve standard C++
exception behavior, such as `std::out_of_range` for bounds-checked access or
`std::invalid_argument` for invalid memory helper arguments.

## Callback exceptions

Callbacks should not throw. When a callback throws, transports catch the
exception, log it, and may transition to an error state depending on the
transport configuration, such as `stop_on_callback_exception`.
