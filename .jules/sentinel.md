## 2025-02-18 - IPv6 Validation Gap
**Vulnerability:** The `is_valid_ipv6` function relied on a restrictive Regex that only supported fully expanded IPv6 addresses, rejecting valid compressed forms (e.g., `::1`, `2001:db8::1`) and IPv4-mapped addresses.
**Learning:** Regex-based validation for complex protocols like IPv6 is often incomplete and prone to errors (false negatives) or ReDoS. Libraries like Boost.Asio provide battle-tested, standard-compliant parsers that should be preferred over manual implementation.
**Prevention:** Use `boost::asio::ip::make_address_v6` (or similar standard library functions) for IP address validation instead of custom regexes.
