# Installation Guide {#user_installation}

This guide covers the supported ways to install and use the **unilink** library in your project. For most users, **vcpkg** is the recommended and simplest option. Container images are available for isolated downstream development environments.

## Prerequisites

- **CMake**: 3.12 or higher for plain builds; 3.21 or higher for the repository presets
- **C++ Compiler**: C++20 compatible (GCC 10+, recent Clang/libc++, MSVC 2022+)
- **Boost**: 1.83.0 or higher
- **Platform**: Linux, Windows, macOS

**Dependency policy:** vcpkg is the recommended dependency supplier. CMake owns the version policy and rejects Boost versions older than 1.83.0. OS package manager Boost packages are supported only when they meet that minimum.

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

**Note:** The vcpkg port name is `jwsung91-unilink`, while the CMake package and target name remain `unilink`.

### Method 2: Install from Source (CMake Package)

Use this method if you prefer not to rely on a package manager or need a custom build.

Source builds still require Boost 1.83.0+. On Ubuntu 22.04/24.04, the default apt Boost package is older than this baseline, so use vcpkg or provide a custom Boost installation through `BOOST_ROOT`/`CMAKE_PREFIX_PATH`.

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

Choose the archive matching your OS and architecture. Replace `${VERSION}` with the current version number (found in the root `CMakeLists.txt`).

- **Linux**: `.tar.gz` (e.g. `unilink-${VERSION}-Linux-x86_64.tar.gz` or `unilink-${VERSION}-Linux-aarch64.tar.gz`)
- **macOS**: `.tar.gz`, `.dmg`
- **Windows**: `.zip`

```bash
# Example for Linux x86_64 (replace ${VERSION} with the actual version, e.g., 0.4.3)
export UNILINK_VERSION="0.4.3"
wget https://github.com/jwsung91/unilink/releases/latest/download/unilink-${UNILINK_VERSION}-Linux-x86_64.tar.gz
tar -xzf unilink-${UNILINK_VERSION}-Linux-x86_64.tar.gz
cd unilink-${UNILINK_VERSION}-Linux-x86_64
```

```bash
# Example for Linux ARM64 / aarch64
export UNILINK_VERSION="0.4.3"
wget https://github.com/jwsung91/unilink/releases/latest/download/unilink-${UNILINK_VERSION}-Linux-aarch64.tar.gz
tar -xzf unilink-${UNILINK_VERSION}-Linux-aarch64.tar.gz
cd unilink-${UNILINK_VERSION}-Linux-aarch64
```

ARM64 release artifacts are intended to be produced from an Ubuntu 22.04 baseline so Jetson/Orin systems can consume the same package without relying on Ubuntu 24.04 userspace.

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

### Method 5: Container Image

Use this method when you want an isolated C++ development environment with
**unilink** already installed.

Container definitions are maintained outside the core repository:

https://github.com/unilink-lab/unilink-containers

Run the core image from your downstream project:

```bash
docker run --rm -it \
  -v "$PWD:/workspace/app" \
  -w /workspace/app \
  ghcr.io/unilink-lab/unilink-core:latest \
  bash
```

Inside the container, configure your project with the installed package prefix:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/unilink
cmake --build build
```

The image sets `UNILINK_ROOT`, `CMAKE_PREFIX_PATH`, `PKG_CONFIG_PATH`, and
`LD_LIBRARY_PATH` for `/opt/unilink`.

## Packaging Notes

- **vcpkg**
  - Official port: `jwsung91-unilink`
  - Recommended for most users
- **Containers**
  - Images and Dockerfiles: https://github.com/unilink-lab/unilink-containers
  - Intended for downstream development and package-consumption checks

Other package managers (e.g., Conan) are not yet officially supported.

## Build Options (Source Builds)

| Option                             | Default | Description                          |
| ---------------------------------- | ------- | ------------------------------------ |
| `UNILINK_BUILD_SHARED`             | `ON`    | Build shared library                 |
| `UNILINK_BUILD_STATIC`             | `ON`    | Build static library                 |
| `UNILINK_BUILD_TESTS`              | `ON`    | Build tests                          |
| `UNILINK_BUILD_DOCS`               | `ON`    | Build documentation                  |
| `UNILINK_ENABLE_CONFIG`            | `ON`    | Enable configuration management API  |
| `UNILINK_ENABLE_INSTALL`           | `ON`    | Enable install targets               |
| `UNILINK_ENABLE_PKGCONFIG`         | `ON`    | Install pkg-config file              |
| `UNILINK_ENABLE_EXPORT_HEADER`     | `ON`    | Generate export header               |

Example:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DUNILINK_BUILD_SHARED=OFF \
  -DUNILINK_BUILD_TESTS=OFF
```

## Next Steps

- [Quick Start Guide](quickstart.md)
- [API Reference](api_guide.md)
- [Examples](https://github.com/unilink-lab/unilink-examples)
