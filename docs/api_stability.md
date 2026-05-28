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
- `send(...)`, `try_send(...)`
- `send_move(...)`, `try_send_move(...)`
- `send_shared(...)`, `try_send_shared(...)`
- TCP/UDP socket tuning builder options

## ABI

C++ ABI stability is not guaranteed before v1.0.

## Diagnostics

`RuntimeStats` is a user-facing diagnostics snapshot. It is not a
synchronization primitive.

## Design-only APIs

`SendResult` is currently design-only unless explicitly implemented in a future
release. It is not part of the runtime API in the current release line.

## Internal headers

Headers under implementation/internal namespaces may change without
compatibility guarantees before v1.0.
