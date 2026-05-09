# System Requirements {#user_requirements}

This guide describes the system requirements and dependencies needed to build and use `unilink`.

---

## System Requirements

### Recommended Platform

- **Ubuntu 22.04 LTS or later**
- **C++20 compatible compiler and standard library** (GCC 10+, recent Clang/libc++, or MSVC 2022+)
- **CMake 3.12 or later** for plain builds; **CMake 3.21 or later** for the repository presets
- **Boost 1.83.0 or later**, preferably supplied by vcpkg

### Supported Platforms

| Platform                   | Status             | Notes                                                  |
| -------------------------- | ------------------ | ------------------------------------------------------ |
| Ubuntu 22.04 LTS           | ✅ Fully Supported | Recommended for production                             |
| Ubuntu 24.04 LTS           | ✅ Fully Supported | Latest features and optimizations                      |
| Ubuntu 22.04 ARM64 (Orin)  | ✅ Validated       | Jetson Orin Nano testbed passed full C++ test sweep    |
| Ubuntu 24.04 ARM64         | 🔄 Validation Path | Secondary ARM64 target in CI/build matrix              |
| Ubuntu 20.04 LTS           | ⚠️ Local Build Only | GCC 10+ required manually                              |
| Other Linux                | 🔄 Should Work     | Not officially tested across all distros/architectures |
| macOS                      | ✅ Fully Supported | Tested in CI (macOS 26, Clang)                         |
| Windows                    | ✅ Fully Supported | Tested in CI (Windows 2022, MSVC)                      |

---

## Dependencies

### Core Library Dependencies

The following packages are required to build and use `unilink`:

Use vcpkg to supply third-party C++ dependencies:

```bash
vcpkg install boost-asio boost-system spdlog
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
```

If you use system packages instead of vcpkg, the selected Boost installation must be 1.83.0 or later. The default Ubuntu 22.04 and 24.04 apt Boost packages do not satisfy this baseline.

### Dependency Details

| Dependency  | Version        | License      | Purpose                                                                   |
| ----------- | -------------- | ------------ | ------------------------------------------------------------------------- |
| **GCC/G++** | 10+            | GPL          | C++20 compiler                                                            |
| **Clang**   | 14+ (optional) | Apache 2.0   | Alternative C++20 compiler                                                |
| **CMake**   | 3.12+ / 3.21+  | BSD-3-Clause | Build system; 3.21+ is needed only for repository presets                 |
| **Boost**   | 1.83.0+        | BSL 1.0      | Asio for async I/O                                                        |

---

## Compiler Requirements

### Minimum Compiler Versions

| Compiler | Minimum Version | Recommended Version |
| -------- | --------------- | ------------------- |
| GCC      | 10.0            | 10.0+               |
| Clang    | 14.0            | Recent Clang/libc++ |

### C++ Standard

- **C++20** is required
- C++23 is supported but not required

---

## Runtime Requirements

### For Applications Using unilink

Applications must be able to resolve the same Boost 1.83.0+ dependency set used to build `unilink`. vcpkg consumers get this through the vcpkg toolchain; source/package consumers should provide a compatible Boost installation through their package environment.

### Thread Support

- POSIX threads (pthread) support required
- Typically included in standard Linux distributions

---

## Platform-Specific Notes

### Ubuntu 22.04 LTS

- Default GCC/Boost packages do **not** meet all requirements
- Install GCC 10+ and use vcpkg or a custom Boost 1.83.0+ installation
- Supported as a build target when those dependencies are supplied explicitly

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
- Must install GCC 10+ or a C++20-capable Clang/libc++ toolchain manually
- See [Ubuntu 20.04 Build Guide](../contributor/build_guide.md#ubuntu-2004-build)
- **Note**: Ubuntu 20.04 reached end-of-life in April 2025; local builds still work

### Other Linux Distributions

- Debian/Fedora/RHEL/Arch builds should work when GCC 10+ and Boost 1.83.0+ are supplied
- CentOS/RHEL 8+: May require SCL or manual compiler installation
- Arch Linux: Fully supported with latest packages

---

## Verifying Your Environment

### Check Compiler Version

```bash
# GCC
g++ --version
# Should show version 10.0 or higher

# Clang (if using)
clang++ --version
# Should show version 14.0 or higher
```

### Check CMake Version

```bash
cmake --version
# Should show 3.12 or higher for plain builds, or 3.21 or higher for repository presets
```

### Check Boost Version

```bash
grep BOOST_LIB_VERSION /path/to/boost/include/boost/version.hpp
# Should show 1_83 or higher
```

### Quick Environment Test

```bash
# Test compilation with C++20
echo '#include <string>
int main() { return 0; }' > test.cpp
g++-10 -std=c++20 test.cpp -o test
./test && echo "C++20 support: OK"
rm test test.cpp
```

---

## Troubleshooting

### Problem: Compiler Too Old

```bash
# Ubuntu 20.04: Install newer GCC
sudo apt install -y gcc-10 g++-10
export CC=gcc-10
export CXX=g++-10
```

### Problem: Boost Not Found

```bash
# Recommended: install dependencies with vcpkg
vcpkg install boost-asio boost-system spdlog

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
