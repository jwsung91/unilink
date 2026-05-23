# API Stability Policy {#user_api_stability}

This document defines which parts of unilink are considered user-facing public API, which parts are supported but advanced, and which parts are internal implementation details.

Before v1.0, unilink aims to preserve source compatibility within the same minor release line where practical, but minor releases may still refine APIs when needed to improve clarity, safety, or runtime predictability.

## Stable Public Surface

The primary public entry point is:

- `<unilink/unilink.hpp>`

The following facade aliases and builder functions are considered user-facing API:

- `unilink::tcp_client(...)`
- `unilink::tcp_server(...)`
- `unilink::udp_client(...)`
- `unilink::udp_server(...)`
- `unilink::serial(...)`
- `unilink::uds_client(...)`
- `unilink::uds_server(...)`

The following wrapper aliases are also user-facing:

- `unilink::TcpClient`
- `unilink::TcpServer`
- `unilink::UdpClient`
- `unilink::UdpServer`
- `unilink::Serial`
- `unilink::UdsClient`
- `unilink::UdsServer`

The following callback context types are user-facing:

- `unilink::MessageContext`
- `unilink::ConnectionContext`
- `unilink::ErrorContext`

The following diagnostics type is user-facing:

- `unilink::RuntimeStats`

## Supported But Advanced Headers

These headers are supported, but most users should prefer including `<unilink/unilink.hpp>`:

- `unilink/builder/*`
- `unilink/wrapper/*`
- `unilink/wrapper/context.hpp`

Use these headers directly only when you need narrower includes or lower-level wrapper control.

## Internal Or Not Source-Stable Before v1.0

The following areas are implementation details or advanced internals and may change before v1.0:

- `unilink/transport/*`
- `unilink/interface/*`
- `unilink/factory/*`
- `unilink/concurrency/*`
- `unilink/config/*`
- `unilink/memory/*`
- Boost adapter headers

Some memory utilities are documented to explain callback data ownership and safety rules. They should still be treated as advanced APIs unless they are exposed through the public facade or callback context types.

## Source Compatibility

Before v1.0, unilink aims to preserve source compatibility within the same minor release line where practical.

However, minor releases may still refine APIs when needed to improve:

- API clarity
- runtime predictability
- memory safety
- packaging correctness
- cross-platform behavior

## ABI Compatibility

C++ ABI compatibility is not guaranteed before v1.0.

Applications and binary packages should be rebuilt against the exact unilink version they consume. Source compatibility is the primary compatibility goal before v1.0.

## Deprecation Policy

Deprecated APIs should normally remain available for at least one minor release before removal.

Exceptions may be made when an API is:

- unsafe
- broken
- misleading
- preventing correct runtime behavior
- incompatible with the documented public API contract

## Python Bindings

Python bindings are maintained separately in `unilink-python`.

This API stability policy applies to the C++ core repository. Python package compatibility is documented in the `unilink-python` repository.

## Recommended Include Policy

Most applications should include:

```cpp
#include <unilink/unilink.hpp>
```

Direct builder or wrapper includes are supported for advanced users, but the umbrella header is the recommended stable entry point for application code.

Move/shared buffer send APIs are user-facing advanced APIs intended for large binary payloads.
