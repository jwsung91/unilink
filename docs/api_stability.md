# API Stability Summary

Full policy:

https://github.com/unilink-lab/unilink-docs/blob/main/docs/user/api_stability.md

## Summary

`unilink` is currently pre-1.0.

## Stable user-facing surface

Prefer:

```cpp
#include <unilink/unilink.hpp>
```

User-facing APIs include:

- builders and wrappers
- `RuntimeStats`
- `MessageContext`, `ConnectionContext`, `ErrorContext`
- `send(...)`, `try_send(...)`
- `send_move(...)`, `try_send_move(...)`
- `send_shared(...)`, `try_send_shared(...)`
- TCP/UDP socket tuning builder options

## Supported but pre-1.0 unstable APIs

These APIs are available, but may still change before v1.0:

- framer APIs
- config APIs
- diagnostics APIs

## ABI

C++ ABI stability is not guaranteed before v1.0.

## Diagnostics

`RuntimeStats` is a user-facing diagnostics snapshot. It is not a
synchronization primitive.

## Design-only APIs

`SendResult` is currently design-only unless explicitly implemented in a future
release. It is not part of the runtime API in the current release line.

## Internal headers

Internal headers are not part of the compatibility guarantee before v1.0.
Prefer the public facade and documented wrapper/builder APIs for application
code.

Compatibility is not guaranteed for deep includes under:

- `transport/*`
- `interface/*`
- `memory/*`
- `concurrency/*`
- `factory/*`

These headers are installed for implementation and advanced integration needs,
but application code should include `<unilink/unilink.hpp>` unless a documented
advanced API requires otherwise.
