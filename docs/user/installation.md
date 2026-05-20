# Installation Guide {#user_installation}

This guide covers the supported ways to install and use the **unilink** library in your project. For most users, **vcpkg** is the recommended and simplest option.

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
project(my_app LANGUAGES CXX)

find_package(unilink CONFIG REQUIRED)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE unilink::unilink)
target_compile_features(my_app PRIVATE cxx_std_20)
```

```cpp
#include <unilink/unilink.hpp>

int main() {
    auto client = unilink::tcp_client("127.0.0.1", 8080).build();
    // Callbacks are optional for construction.
    // Register on_error/on_data for production workflows.
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
target_compile_features(my_app PRIVATE cxx_std_20)
```

### Method 3: Release Packages

Pre-built binary packages are available from GitHub Releases.

#### Step 1: Download and extract

Choose the archive matching your OS and architecture. Release assets use this naming pattern:

```text
unilink-${VERSION}-${OS_LABEL}-${ARCH}.${EXT}
```

`VERSION` is the package version from the release asset name. It normally matches the root `CMakeLists.txt` project version for the GitHub Release. For example, a `v0.1.0` release typically publishes assets named with `0.1.0`.

Common asset names:

| Platform           | Asset                                                   |
| ------------------ | ------------------------------------------------------- |
| Ubuntu 22.04 x64   | `unilink-${VERSION}-ubuntu-22.04-amd64.tar.gz`          |
| Ubuntu 22.04 ARM64 | `unilink-${VERSION}-ubuntu-22.04-arm64.tar.gz`          |
| Ubuntu 24.04 x64   | `unilink-${VERSION}-ubuntu-24.04-amd64.tar.gz`          |
| Ubuntu 24.04 ARM64 | `unilink-${VERSION}-ubuntu-24.04-arm64.tar.gz`          |
| macOS 15 ARM64     | `unilink-${VERSION}-macos-15-arm64.tar.gz`              |
| macOS 15 DMG       | `unilink-${VERSION}-macos-15-arm64.dmg`                 |
| Windows x64        | `unilink-${VERSION}-windows-amd64.zip`                  |
| Windows ARM64      | `unilink-${VERSION}-windows-arm64.zip`                  |

```bash
# Example for Ubuntu 22.04 x64
export UNILINK_VERSION="<latest-release-version>"
wget https://github.com/jwsung91/unilink/releases/latest/download/unilink-${UNILINK_VERSION}-ubuntu-22.04-amd64.tar.gz
tar -xzf unilink-${UNILINK_VERSION}-ubuntu-22.04-amd64.tar.gz
cd unilink-${UNILINK_VERSION}-ubuntu-22.04-amd64
```

```bash
# Example for Ubuntu 22.04 ARM64 / aarch64
export UNILINK_VERSION="<latest-release-version>"
wget https://github.com/jwsung91/unilink/releases/latest/download/unilink-${UNILINK_VERSION}-ubuntu-22.04-arm64.tar.gz
tar -xzf unilink-${UNILINK_VERSION}-ubuntu-22.04-arm64.tar.gz
cd unilink-${UNILINK_VERSION}-ubuntu-22.04-arm64
```

ARM64 release artifacts are intended to be produced from an Ubuntu 22.04 baseline so Jetson/Orin systems can consume the same package without relying on Ubuntu 24.04 userspace.

#### Step 2: Choose an install prefix

Release archives are already laid out as an install prefix. You can use the extracted directory directly or copy it to a stable location.

```bash
# Use the extracted directory directly
export UNILINK_PREFIX="$PWD"

# Or install under /opt/unilink on Unix
sudo mkdir -p /opt/unilink
sudo cp -a include lib share /opt/unilink/
export UNILINK_PREFIX="/opt/unilink"
```

#### Step 3: Use in your project

```cmake
set(CMAKE_PREFIX_PATH "$ENV{UNILINK_PREFIX}")
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
- **Containers**
  - [unilink-lab/unilink-containers](https://github.com/unilink-lab/unilink-containers)

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
