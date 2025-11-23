# Installation Guide

This guide covers different ways to install and use the unilink library in your project.

## Prerequisites

- **CMake**: 3.12 or higher
- **C++ Compiler**: C++17 compatible (GCC 7+, Clang 5+, MSVC 2017+)
- **Boost**: 1.70 or higher (system component required)
- **Platform**: Linux (Ubuntu 18.04+, CentOS 7+, etc.)

## Installation Methods

### Method 1: CMake Package (Recommended)

The easiest way to use unilink is through CMake's package system.

#### Step 1: Install unilink

```bash
# Build and install from source
git clone https://github.com/jwsung91/unilink.git
cd unilink
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build
```

#### Step 2: Use in your project

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.12)
project(my_app CXX)

find_package(unilink CONFIG REQUIRED)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE unilink::unilink)
```

```cpp
// main.cpp
#include <unilink/unilink.hpp>

int main() {
    auto client = unilink::tcp_client("127.0.0.1", 8080).build();
    // ...
}
```

### Method 2: Release Package

Download pre-built packages from GitHub releases.

#### Step 1: Download and extract

```bash
# Download latest release
wget https://github.com/jwsung91/unilink/releases/latest/download/unilink-0.1.5-linux-x64.tar.gz
tar -xzf unilink-0.1.5-linux-x64.tar.gz
cd unilink-0.1.5-linux-x64
```

#### Step 2: Install

```bash
# Install to system
sudo cmake --install .

# Or install to custom prefix
cmake --install . --prefix /opt/unilink
```

#### Step 3: Use in your project

```cmake
# If installed to system
find_package(unilink CONFIG REQUIRED)

# If installed to custom prefix
set(CMAKE_PREFIX_PATH "/opt/unilink")
find_package(unilink CONFIG REQUIRED)
```

### Method 3: Build from Source

For development or custom builds.

#### Step 1: Clone and build

```bash
git clone https://github.com/jwsung91/unilink.git
cd unilink

# Configure with options
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DUNILINK_ENABLE_INSTALL=ON \
    -DUNILINK_ENABLE_PKGCONFIG=ON \
    -DUNILINK_ENABLE_EXPORT_HEADER=ON

# Build
cmake --build build -j
```

#### Step 2: Install

```bash
# Install to system
sudo cmake --install build

# Or install to custom prefix
cmake --install build --prefix /opt/unilink
```

### Method 4: Submodule Integration

For projects that want to include unilink as a submodule.

#### Step 1: Add as submodule

```bash
git submodule add https://github.com/jwsung91/unilink.git third_party/unilink
git submodule update --init --recursive
```

#### Step 2: Use in CMake

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.12)
project(my_app CXX)

# Add unilink subdirectory
add_subdirectory(third_party/unilink)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE unilink::unilink)
```

## Packaging (Conan / vcpkg)

- **Conan recipe**: `packaging/conan`
  - Build/test locally (Conan v2): `conan create packaging/conan --name=unilink --version=0.1.5`
  - Update `conandata.yml` URL/SHA to the release tarball before publishing.
- **vcpkg overlay port**: `packaging/vcpkg/ports/unilink`
  - Consume with overlay: `vcpkg install unilink --overlay-ports=packaging/vcpkg/ports`
  - Keep the portfile pointed at the repo root for local overlay builds.

## Build Options

When building from source, you can configure various options:

| Option | Default | Description |
|--------|---------|-------------|
| `UNILINK_BUILD_SHARED` | `ON` | Build shared library |
| `UNILINK_BUILD_EXAMPLES` | `ON` | Build example programs |
| `UNILINK_BUILD_TESTS` | `ON` | Master test toggle |
| `UNILINK_ENABLE_PERFORMANCE_TESTS` | `OFF` | Build performance tests |
| `UNILINK_BUILD_DOCS` | `ON` | Build documentation |
| `UNILINK_ENABLE_INSTALL` | `ON` | Enable install targets |
| `UNILINK_ENABLE_PKGCONFIG` | `ON` | Install pkg-config file |
| `UNILINK_ENABLE_EXPORT_HEADER` | `ON` | Generate export header |
| `UNILINK_ENABLE_WARNINGS` | `ON` | Enable compiler warnings |
| `UNILINK_ENABLE_SANITIZERS` | `OFF` | Enable sanitizers in Debug |

Example with custom options:

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DUNILINK_BUILD_SHARED=OFF \
    -DUNILINK_ENABLE_SANITIZERS=ON \
    -DUNILINK_BUILD_EXAMPLES=OFF \
    -DUNILINK_BUILD_TESTS=OFF
```

## Verification

After installation, verify that unilink is properly installed:

```bash
# Check CMake package
cmake --find-package -DNAME=unilink -DCOMPILER_ID=GNU -DLANGUAGE=CXX

# Check pkg-config (if enabled)
pkg-config --cflags --libs unilink
```

## Troubleshooting

### Common Issues

1. **CMake can't find unilink**
   ```bash
   # Set CMAKE_PREFIX_PATH
   export CMAKE_PREFIX_PATH="/path/to/unilink/install:$CMAKE_PREFIX_PATH"
   ```

2. **Missing Boost dependency**
   ```bash
   # Ubuntu/Debian
   sudo apt install libboost-dev libboost-system-dev
   
   # CentOS/RHEL
   sudo yum install boost-devel
   ```

3. **Version compatibility**
   - Ensure CMake 3.12+
   - Ensure C++17 compatible compiler
   - Ensure Boost 1.70+

### Getting Help

- Check [Troubleshooting Guide](troubleshooting.md)
- Open an issue on [GitHub](https://github.com/jwsung91/unilink/issues)
- Review [API Documentation](../reference/API_GUIDE.md)

## Next Steps

- [Quick Start Guide](QUICKSTART.md) - Get up and running quickly
- [API Reference](../reference/API_GUIDE.md) - Learn the API
- [Examples](../../examples/) - See practical examples
