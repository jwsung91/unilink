# Transport Channel Contract

> Internal architecture note
>
> This document describes the transport-layer `unilink::interface::Channel` contract and related implementation guarantees. It is not the public wrapper/builder API reference. Public application-facing behavior is documented in `docs/reference/api_guide.md`.

## 1. Introduction
The unilink library provides flexible communication channels (TCP, UDP, Serial, UDS) that abstract away low-level networking and hardware details. To keep behavior consistent across these transports, the transport layer follows a formal "Channel Contract". This document outlines the guarantees expected from internal channel implementations and the runtime assumptions built on top of them.

## 2. Core Principles
The Channel Contract is built upon the following core principles:
*   **Predictability:** Channel behavior should be consistent regardless of the underlying transport.
*   **Robustness:** Channels must handle errors and lifecycle events gracefully, preventing crashes and undefined behavior.
*   **Safety:** Resources must be managed correctly, especially during stop/shutdown sequences.

## 3. Stop Semantics: No Callbacks After Stop()
One of the most critical aspects of the Channel Contract is the "Stop Semantics."

**Contract Rule:**
> **Once `Channel::stop()` is called and returns, no further transport-layer asynchronous callbacks (for example `on_state`, `on_bytes`, `on_backpressure`) related to that specific channel instance shall be invoked.**

This rule ensures that once an application explicitly requests a channel to stop, it can safely clean up resources and assume that no more events will arrive from that channel, preventing potential use-after-free bugs or unexpected state changes.

**Implementation Details:**
*   All asynchronous operations (reads, writes, timers, resolves, accepts) must be cancelled or aborted when `stop()` is initiated.
*   Callback handlers must include guards (e.g., checking `stopping_` or `alive_` flags) to prevent execution if `stop()` has been called.
*   Any terminal state change (e.g., `LinkState::Closed` or `LinkState::Error`) that occurs *as a direct consequence of `stop()` being called* should typically NOT result in a final `on_state` callback, as the caller has already initiated the shutdown and implicitly knows the channel is closing.
*   In scenarios where `stop()` fails to fully complete its asynchronous cleanup (e.g., due to `shared_from_this()` failure in destructors), synchronous cleanup is performed as a fallback, and the guarantee of no further callbacks is maintained.
*   **TcpServerSession**: `alive_` guards are used within asynchronous handlers (`start_read`, `do_write` completion) to prevent callbacks after `do_close()` has been initiated. `do_close()` no longer triggers a backpressure relief callback.

**Transport-Specific Implementations:**

*   **TcpClient:**
    *   `stop_requested_` and `stopping_` flags are used to gate callbacks in `notify_state()`, `report_backpressure()`, and asynchronous handlers.
    *   `perform_stop_cleanup()` explicitly avoids invoking `on_backpressure` callback during shutdown.
*   **Serial:**
    *   `stopping_` flag is used to gate callbacks in `notify_state()`, `report_backpressure()`, and asynchronous handlers.
*   **TcpServer:**
    *   `stopping_` flag is used to gate callbacks in `notify_state()` and asynchronous handlers.
    *   `stop()` method includes a `try-catch` block around `shared_from_this()` to handle cases where the object is being destroyed, ensuring synchronous cleanup without `std::bad_weak_ptr` crashes.
    *   **Callbacks**: Callback member variables (`on_bytes_`, `on_multi_data_`, etc.) are protected by a mutex to prevent data races during registration and invocation.

## 4. Backpressure Policy
To prevent memory exhaustion and manage flow control, channels implement a backpressure mechanism based on the size of the internal write queue.

**Contract Rules:**
1.  **Triggering (High Watermark):** When the size of queued write data (bytes pending transmission) reaches or exceeds the configured `backpressure_threshold`, the transport MUST invoke its internal `on_backpressure` callback with the current queue size. Wrapper APIs may choose not to expose this directly.
    
2.  **Relieving (Low Watermark):** Once backpressure is active, when the queue size drops to or below a "Low Watermark" (typically defined as `threshold / 2` or `0` depending on implementation, but must be strictly less than the high watermark), the transport MUST invoke its internal `on_backpressure` callback with the new lower queue size.

3.  **No Relief on Stop:** As per the **Stop Semantics**, clearing the queue during a `stop()` or shutdown sequence MUST NOT trigger a backpressure relief callback. The application is shutting down the channel and does not need to know that the queue is empty.

**Transport-Specific Implementation Details:**
*   **TcpClient:** Implements a high watermark at `backpressure_threshold` and a low watermark at `threshold / 2` (or 1 if threshold is small). Checks are performed after every write operation (enqueue/dequeue).
*   **Serial:** Should follow the same logic as TcpClient.
*   **TcpServer:** Implements backpressure per-session (`TcpServerSession`).
    *   **Note:** This is a transport concern. The current wrapper/builder API does not expose a public `on_backpressure()` hook.

## 5. Error Handling Consistency
(To be defined in future iterations)

## 6. API Whitelist and State Transitions
(To be defined in future iterations)

## 7. Versioning and Evolution
(To be defined in future iterations)
