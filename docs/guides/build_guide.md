# Build Guide

Complete guide for building `unilink` with different configurations and platforms.

---

## Table of Contents

1. [Quick Build](#quick-build)
2. [Build Configurations](#build-configurations)
3. [Build Options Reference](#build-options-reference)
4. [Platform-Specific Builds](#platform-specific-builds)
5. [Advanced Build Examples](#advanced-build-examples)

---

## Quick Build

### Basic Build (Recommended)

```bash
# 1. Install dependencies
sudo apt update && sudo apt install -y build-essential cmake libboost-dev libboost-system-dev

# 2. Configure and build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# 3. (Optional) Install
sudo cmake --install build
```

---

## Build Configurations

You can build the library with different configurations to optimize for your use case.

### Minimal Build (Builder API only)

**Recommended for most users** - includes only the Builder API with a smaller footprint.

```bash
# Configure for minimal footprint (Builder API only)
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DUNILINK_ENABLE_CONFIG=OFF

# Build
cmake --build build -j
```

**Benefits:**
- ✅ Faster compilation time
- ✅ Smaller binary size (~30% reduction)
- ✅ Lower memory usage
- ✅ Simpler dependencies

**Use for:**
- Simple TCP/Serial applications
- Embedded systems with memory constraints
- Production deployments with minimal footprint

---

### Full Build (includes Configuration Management API)

Includes advanced configuration management features for dynamic configuration.

```bash
# Configure with all features
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DUNILINK_ENABLE_CONFIG=ON

# Build
cmake --build build -j
```

**Benefits:**
- ✅ Dynamic configuration management
- ✅ File-based configuration loading
- ✅ Runtime parameter adjustment
- ✅ Advanced features for complex applications

**Use for:**
- Configuration-heavy applications
- Testing and development
- Applications requiring runtime configuration

---

## Build Options Reference

### Core Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | `Release` | Build type: `Release`, `Debug`, `RelWithDebInfo` |
| `UNILINK_ENABLE_CONFIG` | `ON` | Enable configuration management API |
| `BUILD_EXAMPLES` | `ON` | Build example applications |
| `BUILD_TESTING` | `ON` | Build unit tests |

### Development Options

| Option | Default | Description |
|--------|---------|-------------|
| `UNILINK_ENABLE_MEMORY_TRACKING` | `ON` | Enable memory tracking for debugging |
| `UNILINK_ENABLE_SANITIZERS` | `OFF` | Enable AddressSanitizer and other sanitizers |
| `CMAKE_EXPORT_COMPILE_COMMANDS` | `OFF` | Generate `compile_commands.json` for IDEs |

### Installation Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_INSTALL_PREFIX` | `/usr/local` | Installation directory |
| `UNILINK_INSTALL_DOCS` | `ON` | Install documentation files |

---

## Build Types Comparison

### Release Build (Default)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

- ✅ Full optimizations (-O3)
- ✅ No debug symbols
- ✅ Smallest binary size
- ✅ Best runtime performance
- ⚠️ Harder to debug

**Use for:** Production deployments

---

### Debug Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

- ✅ No optimizations (-O0)
- ✅ Full debug symbols
- ✅ Easier debugging with GDB/LLDB
- ⚠️ Slower runtime performance
- ⚠️ Larger binary size

**Use for:** Development and debugging

---

### RelWithDebInfo Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

- ✅ Optimizations enabled (-O2)
- ✅ Debug symbols included
- ✅ Good balance for profiling
- ⚠️ Larger binary than Release

**Use for:** Performance profiling and production debugging

---

## Advanced Build Examples

### Example 1: Minimal Production Build

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DUNILINK_ENABLE_CONFIG=OFF \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_TESTING=OFF

cmake --build build -j
sudo cmake --install build
```

---

### Example 2: Development Build with Examples

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DUNILINK_ENABLE_CONFIG=ON \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_TESTING=ON \
  -DUNILINK_ENABLE_MEMORY_TRACKING=ON

cmake --build build -j
```

---

### Example 3: Testing with Sanitizers

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DUNILINK_ENABLE_CONFIG=ON \
  -DBUILD_TESTING=ON \
  -DUNILINK_ENABLE_SANITIZERS=ON

cmake --build build -j

# Run tests with memory error detection
cd build && ctest --output-on-failure
```

**Sanitizers detect:**
- Memory leaks
- Use-after-free errors
- Buffer overflows
- Thread race conditions

---

### Example 4: Build with Custom Boost Location

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DBOOST_ROOT=/opt/boost \
  -DBoost_NO_SYSTEM_PATHS=ON

cmake --build build -j
```

---

### Example 5: Build with Specific Compiler

```bash
# Using Clang
export CC=clang-14
export CXX=clang++-14

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# Using specific GCC version
export CC=gcc-11
export CXX=g++-11

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

---

## Platform-Specific Builds

### Ubuntu 22.04 (Recommended)

```bash
# Install dependencies
sudo apt update && apt install -y \
  build-essential cmake libboost-dev libboost-system-dev

# Build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

---

### Ubuntu 20.04 Build

Ubuntu 20.04's default GCC 9.4 does not meet the C++17 requirements. You must install a newer compiler.

#### Prerequisites

```bash
# Install dependencies
sudo apt update && sudo apt install -y \
  build-essential \
  cmake \
  libboost-dev \
  libboost-system-dev

# Install newer compiler
sudo apt install -y gcc-11 g++-11
```

#### Build Steps

```bash
# 1. Set compiler environment
export CC=gcc-11
export CXX=g++-11

# 2. Configure CMake
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=17 \
  -DUNILINK_ENABLE_CONFIG=ON \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_TESTING=ON

# 3. Build
cmake --build build -j $(nproc)

# 4. Run tests (optional)
cd build && ctest --output-on-failure
```

#### Notes
- Ubuntu 20.04 LTS reaches end-of-life in April 2025
- Consider upgrading to Ubuntu 22.04 LTS for better long-term support
- CI/CD support for Ubuntu 20.04 has been temporarily removed due to performance issues

---

### Debian 11+

```bash
# Install dependencies
sudo apt update && apt install -y \
  build-essential cmake libboost-dev libboost-system-dev

# Build (same as Ubuntu 22.04)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

---

### Fedora 35+

```bash
# Install dependencies
sudo dnf install -y \
  gcc-c++ cmake boost-devel

# Build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

---

### Arch Linux

```bash
# Install dependencies
sudo pacman -S base-devel cmake boost

# Build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

---

## Build Performance Tips

### Parallel Builds

Use `-j` flag for parallel compilation:

```bash
# Use all CPU cores
cmake --build build -j

# Use specific number of cores
cmake --build build -j 4
```

### Ccache for Faster Rebuilds

```bash
# Install ccache
sudo apt install -y ccache

# Configure CMake to use ccache
cmake -S . -B build \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build -j
```

### Ninja Build System (Faster than Make)

```bash
# Install ninja
sudo apt install -y ninja-build

# Configure with Ninja
cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Release

# Build with Ninja
cmake --build build
```

---

## Installation

### System-Wide Installation

```bash
# Build first
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# Install (requires root)
sudo cmake --install build

# Library installed to: /usr/local/lib/libunilink.so
# Headers installed to: /usr/local/include/unilink/
```

### Custom Installation Directory

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=$HOME/.local

cmake --build build -j
cmake --install build
```

### Uninstall

```bash
sudo xargs rm < build/install_manifest.txt
```

---

## Verifying the Build

### Run Unit Tests

```bash
cd build
ctest --output-on-failure
```

### Run Examples

```bash
# TCP Echo Server
./build/examples/tcp/single-echo/tcp_echo_server

# TCP Echo Client (in another terminal)
./build/examples/tcp/single-echo/tcp_echo_client
```

### Check Library Symbols

```bash
nm -D build/libunilink.so | grep unilink
```

---

## Troubleshooting

### Problem: CMake Can't Find Boost

```bash
# Specify Boost location
cmake -S . -B build -DBOOST_ROOT=/usr/local

# Or install boost-dev
sudo apt install -y libboost-all-dev
```

### Problem: Compiler Not Found

```bash
# Specify compiler explicitly
cmake -S . -B build \
  -DCMAKE_C_COMPILER=gcc-11 \
  -DCMAKE_CXX_COMPILER=g++-11
```

### Problem: Out of Memory During Build

```bash
# Reduce parallel jobs
cmake --build build -j2

# Or build sequentially
cmake --build build
```

### Problem: Permission Denied During Install

```bash
# Use sudo for system-wide install
sudo cmake --install build

# Or install to user directory
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --build build -j
cmake --install build
```

---

## Next Steps

- [Requirements](requirements.md) - System requirements and dependencies
- [Performance Optimization](performance.md) - Optimize build configuration
- [Testing Guide](testing.md) - Run tests and CI/CD integration
- [Quick Start Guide](QUICKSTART.md) - Start using unilink

