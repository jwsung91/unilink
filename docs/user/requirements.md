# System Requirements {#user_requirements}

This guide describes the system requirements and dependencies needed to build and use `unilink`.

---

## System Requirements

### Recommended Platform

- **Ubuntu 22.04 LTS or later**
- **C++17 compatible compiler** (GCC 11+ or Clang 14+)
- **CMake 3.12 or later**

### Supported Platforms

| Platform         | Status              | Notes                                 |
| ---------------- | ------------------- | ------------------------------------- |
| Ubuntu 22.04 LTS | ✅ Fully Supported  | Recommended for production            |
| Ubuntu 24.04 LTS | ✅ Fully Supported  | Latest features and optimizations     |
| Ubuntu 20.04 LTS | ⚠️ Local Build Only | GCC 11+ required manually             |
| Other Linux      | 🔄 Should Work      | Not officially tested                 |
| macOS            | ✅ Fully Supported  | Tested in CI (macOS 14, Clang)        |
| Windows          | ✅ Fully Supported  | Tested in CI (Windows 2022, MSVC)     |

---

## Dependencies

### Core Library Dependencies

The following packages are required to build and use `unilink`:

```bash
# Ubuntu/Debian
sudo apt update && sudo apt install -y \
  build-essential \
  cmake \
  libboost-dev \
  libboost-system-dev
```

### Dependency Details

| Dependency  | Version        | License      | Purpose                    |
| ----------- | -------------- | ------------ | -------------------------- |
| **GCC/G++** | 11+            | GPL          | C++17 compiler             |
| **Clang**   | 14+ (optional) | Apache 2.0   | Alternative C++17 compiler |
| **CMake**   | 3.12+          | BSD-3-Clause | Build system               |
| **Boost**   | 1.65+          | BSL 1.0      | Asio for async I/O         |

---

## Compiler Requirements

### Minimum Compiler Versions

| Compiler | Minimum Version | Recommended Version |
| -------- | --------------- | ------------------- |
| GCC      | 11.0            | 11.4+               |
| Clang    | 14.0            | 14.0+               |

### C++ Standard

- **C++17** is required
- C++20 and C++23 are supported but not required

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
# Test compilation with C++17
echo 'int main() { return 0; }' > test.cpp
g++ -std=c++17 test.cpp -o test
./test && echo "C++17 support: OK"
rm test test.cpp
```

---

## Troubleshooting

### Problem: Compiler Too Old

```bash
# Ubuntu 20.04: Install newer GCC
sudo apt install -y gcc-11 g++-11
export CC=gcc-11
export CXX=g++-11
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
