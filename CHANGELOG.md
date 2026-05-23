# Changelog

## v0.7.5

### Release Type

Performance-readiness stabilization release.

This release focuses on runtime observability, BestEffort drop visibility, send/backpressure semantics, and performance tuning surfaces ahead of the future v0.8 release line.

### Added

- Added runtime statistics for observing queue pressure and transport behavior.
- Added BestEffort dropped message/byte accounting.
- Added tests for runtime stats and drop accounting.
- Added UDP large-payload delivery validation.
- Added move/shared-buffer send APIs for large payload workflows.
- Added TCP/UDP socket tuning options.
- Added SendResult API design documentation.
- Added transport feature and stats support matrix.

### Changed

- Clarified send and backpressure semantics in user and contributor docs.
- Clarified that local send acceptance does not guarantee remote delivery.
- Clarified that BestEffort may drop older queued payloads to keep newer data fresh.
- Clarified that Reliable `send()` may block under backpressure.
- Improved performance guidance around queue pressure, drops, and tuning.

### Notes

- This release does not declare C++ ABI stability.
- Existing bool-based send APIs remain available.
- `SendResult` is design-only unless separately implemented.
- Benchmark result numbers remain in the benchmark repository, not in the core docs.
- Python bindings remain maintained separately in `unilink-python`.

### Known Limitations

- C++ ABI stability is not guaranteed before v1.0.
- Some internal headers may change before v1.0.
- Benchmark results are environment-specific and are not performance guarantees.
- UDP large-payload behavior should be interpreted according to the core tests and benchmark model notes.
