# Channel Contract

`unilink` channels (TCP/UDP/Serial) follow a single contract so users only learn one set of rules as more transports are added.

## Terminology

- **Channel**: Abstraction over a transport (TCP client, UDP socket, Serial port)
- **User callback**: Functions registered via `on_bytes`, `on_state`, or `on_backpressure`
- **Pump**: Running the `io_context` for a bounded time/step count to process handlers

## LinkState Mapping

| Contract state | `common::LinkState`                           | Notes                                   |
| -------------- | --------------------------------------------- | --------------------------------------- |
| Init           | `Idle`                                        | Immediately after construction          |
| Opening        | `Connecting`, `Listening` (UDP passive start) | Connecting/arming receive/bind          |
| Ready          | `Connected`                                   | Data can flow                           |
| Stopping       | (implicit while `stop()` runs)                | No explicit enum; stop cancels handlers |
| Stopped/Closed | `Closed`                                      | Terminal unless restarted               |
| Error          | `Error`                                       | Terminal until restarted                |

## State Transitions

- `start()`/`open()`: `Idle` → `Connecting`/`Listening` → `Connected` on success
- Error: any state → `Error`
- `stop()`: `Connecting`/`Connected`/`Listening` → (Stopping) → `Closed`
- `stop()` is idempotent and safe to call multiple times.

## Allowed API States

| API                                 | Allowed states                                  | Behavior & notes                                             |
| ----------------------------------- | ----------------------------------------------- | ------------------------------------------------------------ |
| `start()`                           | `Idle`, `Closed`, `Error`                       | No-op if already connecting/connected                        |
| `async_write_*()`                   | `Connected` (TCP/Serial queue while connecting) | Queues send; UDP drops if remote not known                   |
| `on_bytes/on_state/on_backpressure` | Any                                             | Replaceable; callbacks serialized per channel instance       |
| `stop()`                            | Any                                             | Cancels in-flight ops, clears queues, notifies `Closed` once |

## Callback Rules

- **Serialized**: All callbacks for a channel execute on its strand/I/O executor; no concurrent re-entry.
- **No duplicates**: State notifications are emitted once per state change (notably `Error`/`Closed`).
- **Exceptions**:
  - TCP: exceptions in `on_bytes` trigger reconnect via `Connecting`.
  - UDP: exceptions are logged; if `stop_on_callback_exception` is true, state moves to `Error`.
  - Serial: `stop_on_callback_exception` moves to `Error`; otherwise retries according to `reopen_on_error`.
  - Exceptions never escape the handler boundary.

## stop/close Semantics

- Safe to call multiple times and from any thread.
- After `stop()`:
  - User `on_bytes` callbacks are not invoked (canceled reads/writes do not bubble to user).
  - Queues are cleared and backpressure reset; a single `Closed` notification is delivered.

## Backpressure & Queues

- Each channel maintains a send queue with a high/low watermark (`backpressure_threshold` / half).
- `on_backpressure(queued_bytes)` fires on entering and leaving the high watermark region.
- Hard cap: queue growth beyond the limit is **fail-fast** → `Error` (UDP/TCP/Serial aligned).

## Protocol Semantics

- TCP: byte stream (no message boundaries); framing is user responsibility.
- UDP: datagram; truncation (buffer too small) is treated as error and moves to `Error`.
- Serial: byte stream; framing is user responsibility (or higher-level option).

## Channel-Specific Notes

| Channel | Behavior                                                                                                                                                 |
| ------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| UDP     | First inbound packet locks the remote endpoint; later packets from other peers are ignored. Writes with no remote are dropped with a warning (no error). |
| TCP     | Disconnects or `on_bytes` exceptions trigger `Connecting` (retry) unless stopped.                                                                        |
| Serial  | `reopen_on_error` controls retry; `stop_on_callback_exception` governs whether callback exceptions become `Error` or trigger retry.                      |

## Test Coverage

- Contract tests validate: idempotent `stop()`, no user callbacks after stop, single `Error` notification, serialized callbacks (no re-entry), fail-fast backpressure policy, and `open → ready → stop → closed` lifecycle across TCP/Serial/UDP.

## Running Contract Tests

- CI default runs `contract_unit` for fast, network-free regression checks. Coverage/nightly flows can run full `ctest` (or `ctest -L contract_net`) on a network-enabled runner to restore coverage and exercise loopback paths.
- `contract_net` tests may `GTEST_SKIP` in sandboxes where socket bind/connect is blocked; skip messages indicate the reason (e.g., “Socket open not permitted in sandbox”).
