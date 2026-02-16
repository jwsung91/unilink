## 2025-02-14 - Clang Segfault on Manual Compilation
**Learning:** Manual compilation of individual source files (`clang++ file.cc`) in this project can lead to segfaults in `clang::Parser::SkipUntil`, possibly due to missing generated headers (like `unilink_export.hpp`) or complex dependencies not being resolved correctly outside of CMake.
**Action:** Always rely on `cmake` and `ctest` to build and test components, even for small reproduction scripts if possible, or ensure all generated headers and include paths are perfectly set up.
