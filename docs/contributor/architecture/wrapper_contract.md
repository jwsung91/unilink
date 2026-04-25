# Wrapper Contract

**Public wrapper behavior note:** This document describes the intended application-facing behavior of the wrapper layer: `TcpClient`, `TcpServer`, `UdpClient`, `UdpServer`, `Serial`, `UdsClient`, and `UdsServer`. For lower-level transport guarantees, see `docs/architecture/channel_contract.md`.

## Scope

The wrapper layer exists to provide a predictable, transport-agnostic surface over the internal transport implementations.
The goal is not that every transport behaves identically at the protocol level, but that lifecycle and callback behavior remain consistent enough that users can switch transports without relearning control flow.

This document captures the behavioral contract currently enforced by the wrapper implementation and wrapper-focused tests.

## Core Rules

### 1. `start()` reflects real transport state

- `start()` returns `std::future<bool>`.
- The future resolves from an actual transport state transition rather than from an optimistic wrapper-side path.
- `true` means the wrapper reached its active state:
  - clients became connected
  - servers became listening
- `false` means startup failed or the wrapper was stopped before startup completed.

### 2. Repeated `start()` and `stop()` are safe

- Repeated `start()` calls are tolerated.
- Repeated `stop()` calls are tolerated.
- `stop()` is intended to be idempotent from the caller's perspective.
- Shutdown may clear pending startup futures with `false` when startup did not complete.

### 3. `auto_start(true)` follows the same startup contract

- Enabling `auto_start(true)` triggers the same wrapper startup path used by explicit `start()`.
- Auto-managed startup is not a separate fast path with weaker guarantees.
- Callback registration should still happen before enabling `auto_start(true)` or before calling `start()`.

## External `io_context` Contract

### 4. Externally supplied `io_context` can be reused

- Wrappers that accept an external `boost::asio::io_context` must tolerate the context already being in a stopped state.
- Managed wrappers restart a stopped external `io_context` before beginning work.
- This allows restart flows to behave consistently across wrapper families.

### 5. Managed and unmanaged external contexts have different ownership rules

- `manage_external_context(false)` means the wrapper uses the external context but does not own its run loop.
- `manage_external_context(true)` means the wrapper owns the wrapper-specific run loop it starts around that external context.

Expected behavior:

- unmanaged external context:
  - wrapper `stop()` must not stop the caller-owned context
- managed external context:
  - wrapper `stop()` must stop the context/run loop it manages
  - restart after a prior stop must still work

## Callback Contract

### 6. Handler replacement uses the latest callback

- Re-registering `on_connect`, `on_disconnect`, `on_data`, `on_message`, or `on_error` replaces the previous handler.
- Internal callback dispatch uses handler snapshots before invocation.
- The wrapper should not invoke a stale handler after replacement purely because dispatch raced with registration.

### 7. No wrapper callbacks after `stop()` returns

- Once wrapper `stop()` returns, wrapper-level callbacks from that wrapper instance should not continue to reach user code.
- This applies to late state callbacks and late data callbacks arriving from transport cleanup paths.
- Internally, wrappers achieve this by detaching handlers from the transport and guarding asynchronous paths.

### 8. Generic fallback errors are normalized

- When transport-specific error mapping is unavailable, wrappers use stable generic messages.
- Client wrappers use `Connection error`.
- Server wrappers use `Server error`.
- More specific transport error information may still be surfaced when available.

## Transport-Agnostic Expectations

These expectations apply across TCP, UDP, Serial, and UDS wrappers where the concept is meaningful:

- startup resolves from actual readiness, not wrapper intent
- shutdown is safe to repeat
- callback replacement is deterministic
- managed external `io_context` lifecycle is consistent
- late callbacks after shutdown are suppressed

Protocol-specific differences still exist:

- UDP connection semantics are lighter than stream transports
- server wrappers expose multi-client events while client wrappers do not
- transport-specific configuration still varies by builder and wrapper type

Those differences are expected. The contract here is about wrapper control flow and callback safety, not protocol equivalence.

## Testing Status

The wrapper test suite validates this contract through:

- advanced lifecycle tests for client and server wrappers
- injected transport tests for deterministic startup and failure paths
- managed external `io_context` restart and shutdown checks
- callback replacement and late-callback suppression tests

When changing wrapper lifecycle or callback dispatch code, update the related wrapper tests in `test/unit/wrapper/` together with the implementation.
