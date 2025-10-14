# Windows Build Guide

This guide provides step-by-step instructions for building unilink on Windows.

---

## Prerequisites

### Required Software

1. **Visual Studio 2019 or later** (Community Edition is fine)
   - Download from: https://visualstudio.microsoft.com/
   - Install with "Desktop development with C++" workload
   - Includes: MSVC compiler, CMake, Git

2. **CMake 3.12 or later**
   - Included with Visual Studio, or
   - Download from: https://cmake.org/download/

3. **Boost 1.70 or later**
   - **Option A**: Use vcpkg (Recommended)
   - **Option B**: Download pre-built binaries from https://sourceforge.net/projects/boost/

### Optional Tools

- **Conan** (for dependency management): `pip install conan`
- **Git** (if not using Visual Studio installer)

---

## Method 1: Build with Visual Studio (Recommended)

### Step 1: Install Dependencies with vcpkg

```powershell
# Open PowerShell or Developer Command Prompt

# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# Install Boost
.\vcpkg install boost-system:x64-windows
.\vcpkg install boost-asio:x64-windows
```

### Step 2: Clone and Configure

```powershell
# Clone the repository
git clone https://github.com/jwsung91/unilink.git
cd unilink

# Configure with CMake (using vcpkg)
cmake -S . -B build ^
  -DCMAKE_TOOLCHAIN_FILE="[vcpkg root]/scripts/buildsystems/vcpkg.cmake" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DUNILINK_BUILD_TESTS=ON ^
  -DUNILINK_BUILD_EXAMPLES=ON
```

### Step 3: Build

```powershell
# Build the project
cmake --build build --config Release -j

# Or open in Visual Studio
cmake --open build
```

### Step 4: Run Tests (Optional)

```powershell
cd build
ctest -C Release --output-on-failure
```

### Step 5: Install (Optional)

```powershell
# Install to default location (requires admin)
cmake --install build --config Release

# Or install to custom location
cmake --install build --config Release --prefix C:\unilink
```

---

## Method 2: Build with Command Line (Advanced)

### Step 1: Setup Environment

```batch
:: Open "Developer Command Prompt for VS 2019/2022"
:: This sets up MSVC compiler environment
```

### Step 2: Install Boost Manually

```powershell
# Download Boost from https://www.boost.org/
# Extract to C:\boost_1_83_0

# Set environment variable
set BOOST_ROOT=C:\boost_1_83_0
```

### Step 3: Build

```batch
:: Clone repository
git clone https://github.com/jwsung91/unilink.git
cd unilink

:: Configure
cmake -S . -B build ^
  -G "Visual Studio 16 2019" ^
  -A x64 ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DBOOST_ROOT=C:\boost_1_83_0 ^
  -DUNILINK_BUILD_TESTS=ON

:: Build
cmake --build build --config Release -j

:: Run tests
cd build
ctest -C Release
```

---

## Method 3: Build with MinGW (Alternative)

### Step 1: Install MinGW-w64

```powershell
# Install via MSYS2 (Recommended)
# Download from: https://www.msys2.org/

# In MSYS2 terminal:
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-boost
```

### Step 2: Build

```bash
# In MSYS2 MinGW64 terminal
git clone https://github.com/jwsung91/unilink.git
cd unilink

# Configure
cmake -S . -B build \
  -G "MinGW Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DUNILINK_BUILD_TESTS=ON

# Build
cmake --build build -j

# Run tests
cd build
ctest --output-on-failure
```

---

## Method 4: Build with Conan (Package Manager)

### Step 1: Install Conan

```powershell
pip install conan

# Setup Conan profile for Visual Studio
conan profile detect --force
```

### Step 2: Build

```powershell
# Clone repository
git clone https://github.com/jwsung91/unilink.git
cd unilink

# Install dependencies with Conan
conan install . --output-folder=build --build=missing

# Configure
cmake -S . -B build ^
  -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake ^
  -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release -j

# Run tests
cd build
ctest -C Release
```

---

## Build Options

### Common CMake Options

```powershell
cmake -S . -B build ^
  -DUNILINK_BUILD_SHARED=ON          # Build shared library (default: ON)
  -DUNILINK_BUILD_STATIC=ON          # Build static library (default: ON)
  -DUNILINK_BUILD_TESTS=ON           # Build tests (default: ON)
  -DUNILINK_BUILD_EXAMPLES=ON        # Build examples (default: ON)
  -DUNILINK_BUILD_DOCS=OFF           # Build documentation (default: ON)
  -DUNILINK_ENABLE_CONFIG=ON         # Enable config API (default: ON)
  -DUNILINK_ENABLE_WARNINGS=ON       # Enable compiler warnings (default: ON)
  -DUNILINK_ENABLE_WERROR=OFF        # Treat warnings as errors (default: OFF)
```

### Visual Studio Specific

```powershell
# Choose generator
-G "Visual Studio 16 2019"    # VS 2019
-G "Visual Studio 17 2022"    # VS 2022

# Choose platform
-A x64                        # 64-bit (recommended)
-A Win32                      # 32-bit

# Choose toolset
-T v142                       # VS 2019 toolset
-T v143                       # VS 2022 toolset
```

---

## Troubleshooting

### Error: Boost not found

```powershell
# Solution 1: Use vcpkg
.\vcpkg install boost-system:x64-windows

# Solution 2: Set BOOST_ROOT
set BOOST_ROOT=C:\path\to\boost
cmake -S . -B build -DBOOST_ROOT=%BOOST_ROOT%

# Solution 3: Use Conan
conan install . --build=missing
```

### Error: ws2_32.lib not found

```powershell
# Make sure you're using Developer Command Prompt
# Or install Windows SDK
```

### Error: C++ standard not supported

```powershell
# Update Visual Studio to latest version
# Or use newer toolset:
cmake -S . -B build -T v143
```

### Serial Port Issues on Windows

Windows uses different serial port naming:
- Linux: `/dev/ttyUSB0`, `/dev/ttyS0`
- Windows: `COM1`, `COM2`, `\\.\COM10` (for COM10+)

```cpp
// Example: Windows serial port usage
auto serial = unilink::serial("COM3", 115200).build();

// For COM ports > 9, use extended format
auto serial = unilink::serial("\\\\.\\COM10", 115200).build();
```

---

## Running Examples

### TCP Server Example

```powershell
cd build\examples\tcp\single-echo\Release
.\echo_tcp_server.exe
```

### TCP Client Example

```powershell
cd build\examples\tcp\single-echo\Release
.\echo_tcp_client.exe
```

### Serial Example

```powershell
cd build\examples\serial\echo\Release
.\echo_serial.exe COM3 115200
```

---

## Using unilink in Your Windows Project

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.12)
project(my_app CXX)

# Find unilink
find_package(unilink CONFIG REQUIRED)

# Create executable
add_executable(my_app main.cpp)

# Link unilink
target_link_libraries(my_app PRIVATE unilink::unilink)

# Set C++ standard
set_target_properties(my_app PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
```

### Build Your Project

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\unilink"
cmake --build build --config Release
```

---

## Performance Optimization

### Release Build with Optimizations

```powershell
cmake -S . -B build ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DUNILINK_ENABLE_LTO=ON          # Link-time optimization
  
cmake --build build --config Release
```

### Static Build (No DLL Dependencies)

```powershell
cmake -S . -B build ^
  -DUNILINK_BUILD_SHARED=OFF ^
  -DUNILINK_BUILD_STATIC=ON ^
  -DBoost_USE_STATIC_LIBS=ON
  
cmake --build build --config Release
```

---

## Next Steps

- [Quick Start Guide](QUICKSTART.md) - Get started with examples
- [API Reference](../reference/API_GUIDE.md) - Complete API documentation
- [Troubleshooting](troubleshooting.md) - Common issues and solutions
- [Examples](../../examples/) - More example projects

---

**Need help?** Open an issue on [GitHub](https://github.com/jwsung91/unilink/issues)

