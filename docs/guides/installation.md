# Installation Guide

This guide covers the supported ways to install and use the **unilink** library in your project. For most users, **vcpkg** is the recommended and simplest option.

## Prerequisites

- **CMake**: 3.12 or higher
- **C++ Compiler**: C++17 compatible (GCC 7+, Clang 5+, MSVC 2017+)
- **Platform**: Linux, Windows, macOS

> Note: When using a package manager (vcpkg), dependencies such as Boost are handled automatically.

## Installation Methods

### Method 1: vcpkg (Recommended)

The easiest and most reliable way to consume **unilink** is via **vcpkg**, which provides a fully integrated CMake workflow and cross-platform builds.

#### Step 1: Install via vcpkg

```bash
vcpkg install jwsung91-unilink
```

#### Step 2: Use in your project

```cmake
cmake_minimum_required(VERSION 3.12)
project(my_app CXX)

find_package(unilink CONFIG REQUIRED)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE unilink::unilink)
```

```cpp
#include <unilink/unilink.hpp>

int main() {
    auto client = unilink::tcp_client("127.0.0.1", 8080).build();
    // ...
}
```

> Note: The vcpkg port name is `jwsung91-unilink`, while the CMake package and target name remain `unilink`.

### Method 2: Install from Source (CMake Package)

Use this method if you prefer not to rely on a package manager or need a custom build.

#### Step 1: Build and install

```bash
git clone https://github.com/jwsung91/unilink.git
cd unilink
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build
```

#### Step 2: Use in your project

```cmake
find_package(unilink CONFIG REQUIRED)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE unilink::unilink)
```

### Method 3: Release Packages

Pre-built binary packages are available from GitHub Releases.

#### Step 1: Download and extract

Choose the archive matching your OS and architecture:

- **Linux**: `.tar.gz` (e.g. `unilink-0.2.0-Linux-x86_64.tar.gz`)
- **macOS**: `.tar.gz`, `.dmg`
- **Windows**: `.zip`

```bash
wget https://github.com/jwsung91/unilink/releases/latest/download/unilink-<version>-Linux-x86_64.tar.gz
tar -xzf unilink-<version>-Linux-x86_64.tar.gz
cd unilink-<version>-Linux-x86_64.tar.gz
```

#### Step 2: Install

```bash
# System install
sudo cmake --install .

# Custom prefix
cmake --install . --prefix /opt/unilink
```

#### Step 3: Use in your project

```cmake
set(CMAKE_PREFIX_PATH "/opt/unilink")
find_package(unilink CONFIG REQUIRED)
```

### Method 4: Git Submodule Integration

For projects that want to vendor **unilink** directly.

#### Step 1: Add submodule

```bash
git submodule add https://github.com/jwsung91/unilink.git third_party/unilink
git submodule update --init --recursive
```

#### Step 2: Use in CMake

```cmake
add_subdirectory(third_party/unilink)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE unilink::unilink)
```

## Packaging Notes

- **vcpkg**
  - Official port: `jwsung91-unilink`
  - Recommended for most users

Other package managers (e.g., Conan) are planned but not yet officially supported.

## Build Options (Source Builds)

| Option                         | Default | Description                |
|--------------------------------|---------|----------------------------|
| `UNILINK_BUILD_SHARED`         | `ON`    | Build shared library       |
| `UNILINK_BUILD_EXAMPLES`       | `ON`    | Build example programs     |
| `UNILINK_BUILD_TESTS`          | `ON`    | Build tests                |
| `UNILINK_BUILD_DOCS`           | `ON`    | Build documentation        |
| `UNILINK_ENABLE_INSTALL`       | `ON`    | Enable install targets     |
| `UNILINK_ENABLE_PKGCONFIG`     | `ON`    | Install pkg-config file    |
| `UNILINK_ENABLE_EXPORT_HEADER` | `ON`    | Generate export header     |

Example:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DUNILINK_BUILD_SHARED=OFF \
  -DUNILINK_BUILD_EXAMPLES=OFF \
  -DUNILINK_BUILD_TESTS=OFF
```

## Next Steps

- [Quick Start Guide](QUICKSTART.md)
- [API Reference](../reference/API_GUIDE.md)
- [Examples](../../examples/)
