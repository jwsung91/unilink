# Channel Contract: Ensuring Predictable and Robust Communication

## 1. Introduction
The Unilink library provides flexible communication channels (TCP, UDP, Serial) that abstract away low-level networking and hardware details. To ensure consistent, predictable, and robust behavior across these diverse transports, a formal "Channel Contract" is established. This document outlines the fundamental rules and guarantees that all Unilink channels must adhere to. Adherence to this contract is critical for building reliable applications and simplifying the interaction with various communication mechanisms.

## 2. Core Principles
The Channel Contract is built upon the following core principles:
*   **Predictability:** Channel behavior should be consistent regardless of the underlying transport.
*   **Robustness:** Channels must handle errors and lifecycle events gracefully, preventing crashes and undefined behavior.
*   **Safety:** Resources must be managed correctly, especially during stop/shutdown sequences.

## 3. Stop Semantics: No Callbacks After Stop()
One of the most critical aspects of the Channel Contract is the "Stop Semantics."

**Contract Rule:**
> **Once `Channel::stop()` is called and returns, no further asynchronous callbacks (e.g., `on_state`, `on_bytes`, `on_backpressure`) related to that specific channel instance shall be invoked.**

This rule ensures that once an application explicitly requests a channel to stop, it can safely clean up resources and assume that no more events will arrive from that channel, preventing potential use-after-free bugs or unexpected state changes.

**Implementation Details:**
*   All asynchronous operations (reads, writes, timers, resolves, accepts) must be cancelled or aborted when `stop()` is initiated.
*   Callback handlers must include guards (e.g., checking `stopping_` or `stop_requested_` flags) to prevent execution if `stop()` has been called.
*   Any terminal state change (e.g., `LinkState::Closed` or `LinkState::Error`) that occurs *as a direct consequence of `stop()` being called* should typically NOT result in a final `on_state` callback, as the caller has already initiated the shutdown and implicitly knows the channel is closing.
*   In scenarios where `stop()` fails to fully complete its asynchronous cleanup (e.g., due to `shared_from_this()` failure in destructors), synchronous cleanup is performed as a fallback, and the guarantee of no further callbacks is maintained.

**Transport-Specific Implementations:**

*   **TcpClient:**
    *   `stop_requested_` and `stopping_` flags are used to gate callbacks in `notify_state()`, `report_backpressure()`, and asynchronous handlers.
    *   `perform_stop_cleanup()` explicitly avoids invoking `on_backpressure` callback during shutdown.
*   **Serial:**
    *   `stopping_` flag is used to gate callbacks in `notify_state()`, `report_backpressure()`, and asynchronous handlers.
*   **TcpServer:**
    *   `stopping_` flag is used to gate callbacks in `notify_state()` and asynchronous handlers.
    *   `stop()` method includes a `try-catch` block around `shared_from_this()` to handle cases where the object is being destroyed, ensuring synchronous cleanup without `std::bad_weak_ptr` crashes.

## 4. Backpressure Policy
(To be defined in future iterations)

## 5. Error Handling Consistency
(To be defined in future iterations)

## 6. API Whitelist and State Transitions
(To be defined in future iterations)

## 7. Versioning and Evolution
(To be defined in future iterations)
