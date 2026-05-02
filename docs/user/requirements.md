# System Requirements {#user_requirements}

This guide describes the system requirements and dependencies needed to build and use `unilink`.

---

## System Requirements

### Recommended Platform

- **Ubuntu 22.04 LTS or later**
- **C++20 compatible compiler and standard library with `std::format` support** (GCC 13+, recent Clang/libc++, or MSVC 2022+)
- **CMake 3.12 or later**

### Supported Platforms

| Platform                   | Status             | Notes                                                  |
| -------------------------- | ------------------ | ------------------------------------------------------ |
| Ubuntu 22.04 LTS           | ✅ Fully Supported | Recommended for production                             |
| Ubuntu 24.04 LTS           | ✅ Fully Supported | Latest features and optimizations                      |
| Ubuntu 22.04 ARM64 (Orin)  | ✅ Validated       | Jetson Orin Nano testbed passed full C++ test sweep    |
| Ubuntu 24.04 ARM64         | 🔄 Validation Path | Secondary ARM64 target in CI/build matrix              |
| Ubuntu 20.04 LTS           | ⚠️ Local Build Only | GCC 13+ required manually                              |
| Other Linux                | 🔄 Should Work     | Not officially tested across all distros/architectures |
| macOS                      | ✅ Fully Supported | Tested in CI (macOS 14, Clang)                         |
| Windows                    | ✅ Fully Supported | Tested in CI (Windows 2022, MSVC)                      |

---

## Dependencies

### Core Library Dependencies

The following packages are required to build and use `unilink`:

```bash
# Ubuntu/Debian
sudo apt update && sudo apt install -y \
  build-essential \
  cmake \
  gcc-13 \
  g++-13 \
  libboost-dev \
  libboost-system-dev
```

### Dependency Details

| Dependency  | Version        | License      | Purpose                                                                   |
| ----------- | -------------- | ------------ | ------------------------------------------------------------------------- |
| **GCC/G++** | 13+            | GPL          | C++20 compiler with `std::format` support                                 |
| **Clang**   | 14+ (optional) | Apache 2.0   | Alternative C++20 compiler; requires a standard library with `std::format` |
| **CMake**   | 3.12+          | BSD-3-Clause | Build system                                                              |
| **Boost**   | 1.65+          | BSL 1.0      | Asio for async I/O                                                        |

---

## Compiler Requirements

### Minimum Compiler Versions

| Compiler | Minimum Version | Recommended Version |
| -------- | --------------- | ------------------- |
| GCC      | 13.0            | 13.0+               |
| Clang    | 14.0            | Recent Clang/libc++ |

### C++ Standard

- **C++20** is required
- `std::format` support in the selected standard library is required
- C++23 is supported but not required

---

## Runtime Requirements

### For Applications Using unilink

```bash
# Install runtime libraries only
sudo apt install -y libboost-system-dev
```

### Thread Support

- POSIX threads (pthread) support required
- Typically included in standard Linux distributions

---

## Platform-Specific Notes

### Ubuntu 22.04 LTS

- Default GCC 11.4 meets all requirements
- All features fully supported
- Recommended platform for production use

### Ubuntu ARM64 / Jetson Orin Nano

- Source builds use the same Linux/POSIX code path as x86_64
- Validated on Jetson Orin Nano with Ubuntu 22.04 on `aarch64`
- Current secondary validation target: Ubuntu 24.04 on `aarch64`
- Full C++ test sweep passed on the Orin Nano testbed: 481 tests passed, 0 failed
- Python bindings validation passed, including import smoke, API tests, and loopback-enabled TCP checks
- ARM64 release packaging validation passed, including installed-package consumer smoke via `find_package(unilink)` and `unilink::unilink`
- One test remains intentionally disabled in that run: `UdsErrorTest.ServerStopWithActiveSessions`
- Serial integration tests require `socat` or physical loopback hardware

### Ubuntu 20.04 LTS

- Default GCC 9.4 does **not** meet requirements
- Must install GCC 11+ or Clang 14+ manually
- See [Ubuntu 20.04 Build Guide](../contributor/build_guide.md#ubuntu-2004-build)
- **Note**: Ubuntu 20.04 reached end-of-life in April 2025; local builds still work

### Other Linux Distributions

- Debian 11+: Supported with default packages
- Fedora 35+: Supported with default packages
- CentOS/RHEL 8+: May require SCL or manual compiler installation
- Arch Linux: Fully supported with latest packages

---

## Verifying Your Environment

### Check Compiler Version

```bash
# GCC
g++ --version
# Should show version 11.0 or higher

# Clang (if using)
clang++ --version
# Should show version 14.0 or higher
```

### Check CMake Version

```bash
cmake --version
# Should show version 3.12 or higher
```

### Check Boost Version

```bash
dpkg -l | grep libboost
# Should show boost libraries version 1.65 or higher
```

### Quick Environment Test

```bash
# Test compilation with C++20 std::format
echo '#include <format>
#include <string>
int main() { return std::format("{}", 42) == "42" ? 0 : 1; }' > test.cpp
g++-13 -std=c++20 test.cpp -o test
./test && echo "C++20 std::format support: OK"
rm test test.cpp
```

---

## Troubleshooting

### Problem: Compiler Too Old

```bash
# Ubuntu 20.04: Install newer GCC
sudo apt install -y gcc-13 g++-13
export CC=gcc-13
export CXX=g++-13
```

### Problem: Boost Not Found

```bash
# Install Boost development packages
sudo apt install -y libboost-all-dev

# Or specify Boost location to CMake
cmake -DBOOST_ROOT=/path/to/boost ...
```

### Problem: CMake Too Old

```bash
# Install latest CMake from official repository
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt update
sudo apt install cmake
```

---

## Next Steps

- [Quick Start Guide](quickstart.md) - Get started with unilink
- [Installation Guide](installation.md) - Installation options
- [Troubleshooting](troubleshooting.md) - Common issues and solutions
