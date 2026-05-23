# SendResult API Design {#contrib_design_send_result}

## Problem Statement

The current `bool send(...)` and `bool try_send(...)` APIs are simple and convenient, but they cannot explain why a send operation succeeded or failed.

This is especially important for backpressure-aware transports:

- `BestEffort` may accept a new payload while dropping older queued payloads.
- `Reliable` may reject or block when the local queue cannot accept more data.
- UDP may reject sends when no remote endpoint is configured.
- A successful local send acceptance does not guarantee remote delivery.

A richer result type would let applications distinguish local acceptance, queue drops, rejection, and transport state failures without replacing the existing bool APIs.

## Current bool API

Current APIs:

```cpp
bool send(std::string_view data);
bool try_send(std::string_view data);
```

Current behavior:

- `true` means the payload was accepted into the local send path.
- `false` means the payload was not accepted.
- `true` does not guarantee remote delivery.
- `true` may still coincide with older queued payloads being dropped under `BestEffort`.

## Goals

- Preserve existing `bool send(...)` APIs.
- Provide an advanced API that explains send outcomes.
- Distinguish rejected sends from dropped queued payloads.
- Make BestEffort behavior observable per send call.
- Keep Reliable and BestEffort semantics consistent with `RuntimeStats`.
- Avoid requiring users to parse logs or compare stats snapshots for every send.

## Non-Goals

- Do not replace existing `send(...)` / `try_send(...)`.
- Do not guarantee remote delivery.
- Do not add application-level acknowledgements.
- Do not make UDP reliable.
- Do not expose transport-internal queue implementation details.
- Do not change default Reliable / BestEffort behavior.

## Proposed API

### Type Definitions

```cpp
namespace unilink {

enum class SendStatus {
  Queued,
  QueuedAfterDroppingOldData,
  RejectedBackpressure,
  Disconnected,
  Closed,
  Stopping,
  InvalidPayload,
  PayloadTooLarge,
  NoRemoteEndpoint,
  TransportError,
};

struct SendResult {
  SendStatus status = SendStatus::TransportError;

  size_t accepted_bytes = 0;
  size_t dropped_bytes = 0;
  size_t queued_bytes = 0;

  uint64_t dropped_messages = 0;

  bool accepted() const noexcept {
    return status == SendStatus::Queued ||
           status == SendStatus::QueuedAfterDroppingOldData;
  }

  bool failed() const noexcept {
    return !accepted();
  }
};

}  // namespace unilink
```

### API Names

Proposed channel-style APIs:

```cpp
SendResult try_send_ex(std::string_view data);
SendResult send_ex(std::string_view data);
```

If move/shared buffer send APIs are part of the active public surface, extended variants should use the same ownership rules:

```cpp
SendResult try_send_move_ex(std::vector<uint8_t>&& data);
SendResult send_move_ex(std::vector<uint8_t>&& data);

SendResult try_send_shared_ex(std::shared_ptr<const std::vector<uint8_t>> data);
SendResult send_shared_ex(std::shared_ptr<const std::vector<uint8_t>> data);
```

The `_ex` suffix is provisional. Alternatives include `send_result(...)`, `try_send_result(...)`, or returning `SendResult` from new advanced APIs under a diagnostics namespace.

## SendStatus Semantics

| Status | Meaning | `accepted()` | RuntimeStats effect |
|--------|---------|-------------:|---------------------|
| `Queued` | New payload accepted without dropping older queued data | true | accepted counters increase |
| `QueuedAfterDroppingOldData` | New payload accepted and older queued payloads were dropped by BestEffort | true | accepted + dropped counters increase |
| `RejectedBackpressure` | New payload rejected because queue limits would be exceeded | false | `failed_sends` increases |
| `Disconnected` | Channel is not connected | false | `failed_sends` increases |
| `Closed` | Channel is closed | false | `failed_sends` increases |
| `Stopping` | Channel is stopping or stop was requested | false | `failed_sends` increases |
| `InvalidPayload` | Empty or otherwise invalid payload | false | `failed_sends` increases |
| `PayloadTooLarge` | Payload exceeds maximum allowed size | false | `failed_sends` increases |
| `NoRemoteEndpoint` | UDP send requested without a known remote endpoint | false | `failed_sends` increases |
| `TransportError` | Other transport-level failure | false | `failed_sends` increases |

## Reliable Semantics

In `Reliable` mode:

- `try_send_ex(...)` should not block.
- `send_ex(...)` may wait for backpressure to clear, matching existing `send(...)` behavior.
- If the queue cannot accept the new payload, the result should be `RejectedBackpressure`.
- Reliable should not report `QueuedAfterDroppingOldData` because it does not intentionally drop older queued payloads to preserve freshness.

## BestEffort Semantics

In `BestEffort` mode:

- New payloads may be accepted while older queued payloads are discarded.
- If no older queued payloads were dropped, the result should be `Queued`.
- If older queued payloads were dropped, the result should be `QueuedAfterDroppingOldData`.
- `dropped_messages` and `dropped_bytes` in `SendResult` should reflect only the drop caused by this send call.
- `RuntimeStats` cumulative drop counters should also increase.

## Relationship To RuntimeStats

`SendResult` is a per-call result.

`RuntimeStats` is a cumulative diagnostic snapshot.

They should use the same semantic categories:

- accepted
- sent
- received
- `failed_sends`
- `dropped_messages`
- `dropped_bytes`
- `queued_bytes`
- `backpressure_events`

`SendResult::dropped_bytes` describes drops caused by the specific send call. `RuntimeStats::dropped_bytes` is cumulative.

`SendResult::accepted_bytes` describes bytes accepted by the specific send call. It does not imply that the remote peer received those bytes.

## API Compatibility

The existing bool APIs remain source-compatible:

```cpp
bool send(std::string_view data);
bool try_send(std::string_view data);
```

The proposed extended APIs are additive.

Existing bool APIs can be implemented in terms of extended APIs later:

```cpp
bool try_send(std::string_view data) {
  return try_send_ex(data).accepted();
}
```

However, this should only be done if behavior remains source-compatible and performance overhead is acceptable.

## Server Send Results

Single-target sends can use the same `SendResult`:

```cpp
SendResult send_to_ex(uint64_t client_id, std::string_view data);
SendResult try_send_to_ex(uint64_t client_id, std::string_view data);
```

Broadcast is more complex because different clients may produce different outcomes.

Possible aggregate result:

```cpp
struct BroadcastSendResult {
  size_t attempted_clients = 0;
  size_t accepted_clients = 0;
  size_t failed_clients = 0;

  size_t accepted_bytes = 0;
  size_t dropped_bytes = 0;
  uint64_t dropped_messages = 0;
};
```

Open question: whether broadcast should return one aggregate result or a map of client id to `SendResult`.

Move/shared buffer server APIs need the same distinction:

- single-target move sends can return one `SendResult`
- single-target shared sends can return one `SendResult`
- shared broadcast sends may need aggregate or per-client results
- move broadcast should remain out of scope because one moved buffer cannot be transferred to multiple clients

## Migration Plan

### Phase 1: Design

Document the semantics and collect feedback.

### Phase 2: Experimental API

Add extended APIs with an experimental name or namespace.

Possible names:

- `try_send_ex`
- `try_send_result`
- `diagnostic_try_send`

### Phase 3: Official 0.8 API

Promote the selected API to user-facing documentation for the 0.8 release line.

### Phase 4: Long-Term

Keep bool APIs as convenience wrappers. Do not remove them.

## Open Questions

1. Should empty payload be `InvalidPayload` and increment `failed_sends`, or should it be ignored without counting as a failed send?
2. Should `send_ex(...)` be allowed to block in Reliable mode, or should all extended APIs be non-blocking?
3. Should the API use `_ex`, `_result`, or a diagnostics namespace?
4. Should `SendResult` include an `ErrorCode`?
5. Should `SendResult` include `pending_bytes`?
6. Should server `broadcast_*` APIs return one aggregate result or per-client results?
7. Should UDP `NoRemoteEndpoint` be a distinct status or a transport error?
8. Should `queued_bytes` report only the primary outgoing queue, or should it include pending Reliable bytes?
9. Should `SendResult` expose a human-readable diagnostic string, or keep that responsibility in logs and `ErrorContext`?
