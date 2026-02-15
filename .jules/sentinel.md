## 2026-02-05 - [DoS: Tight Loop in Async Accept]
**Vulnerability:** A Denial of Service (DoS) vulnerability where the TCP server would enter a tight loop consuming 100% CPU when the client limit was reached. The server would reject a connection (close socket) and immediately restart the async accept operation, processing the pending OS backlog as fast as possible without backoff.
**Learning:** In Boost.Asio (and async IO in general), rejecting a connection and immediately re-issuing an accept call creates a spin loop if the backlog is full. The OS is ready to hand over the next connection instantly.
**Prevention:** Implement a "pause" mechanism. When the server is full (or overloaded), stop issuing new `async_accept` calls. Resume accepting only when resources become available (e.g., a client disconnects). This allows the OS backlog to fill up naturally, eventually causing the OS to drop SYN packets, which is the correct backpressure behavior for a saturated server.

## 2026-02-06 - [Path Traversal: Insecure Device Path Validation]
**Vulnerability:** The `InputValidator::is_valid_device_path` function allowed any Unix path starting with `/` to be considered a valid "device path" if it contained alphanumeric characters, `_`, and `-`. This could allow an application to interact with arbitrary files (e.g., `/etc/passwd`) as if they were serial devices.
**Learning:** Checking only for "safe characters" in a path is insufficient to prevent path traversal or accessing sensitive files when the intent is to restrict access to device nodes. Validating the prefix (e.g., `/dev/`) is crucial for security in this context.
**Prevention:** Enforce strict prefix validation for device paths (e.g., must start with `/dev/` on Unix). Reject general file system paths unless explicitly allowed.

## 2026-02-07 - [Input Validation: Config Objects Should Validate Types, Not Just Non-Empty]
**Vulnerability:** `TcpClientConfig` relied on `!host.empty()` for validation, allowing invalid strings (like URLs or command injection payloads) to be passed down to the network layer.
**Learning:** Configuration objects are often the first line of defense. Relying on "it will fail later" (e.g., in `asio::resolver`) misses an opportunity to catch malicious or malformed input early and provide clear feedback.
**Prevention:** Expose granular validation logic (like `is_valid_host`, `is_valid_ipv4`) in utility classes and use them directly in configuration object `is_valid()` checks.

## 2026-02-14 - [Algorithmic Complexity DoS: Quadratic Search in Packet Framer]
**Vulnerability:** `PacketFramer::push_bytes` performed a full buffer scan for the end pattern on every new data chunk, leading to $O(N^2)$ complexity when data arrives byte-by-byte. This allowed an attacker to consume excessive CPU by slowly sending large packets.
**Learning:** Re-scanning data that has already been checked is a common source of algorithmic complexity vulnerabilities in parsers and framers. Even if the maximum buffer size is bounded, repeated scans can be catastrophic.
**Prevention:** Maintain state (e.g., `scanned_idx_`) to track the progress of the search and only scan new data (plus a small overlap for split patterns). Ensure linear $O(N)$ complexity for streaming data processing.
