# Windows Build Script for unilink (PowerShell)
# Copyright 2025 Jinwoo Sung

param(
    [switch]$Debug,
    [switch]$Release,
    [switch]$NoTests,
    [switch]$NoExamples,
    [string]$InstallPrefix = "",
    [string]$Generator = "",
    [switch]$UseVcpkg,
    [string]$VcpkgRoot = "",
    [switch]$SetupVcpkg,
    [switch]$Help
)

# Show help
if ($Help) {
    Write-Host @"
Windows Build Script for unilink

Usage: .\build_windows.ps1 [options]

Options:
  -Debug              Build in Debug mode
  -Release            Build in Release mode (default)
  -NoTests            Disable building tests
  -NoExamples         Disable building examples
  -InstallPrefix PATH Install to specified path after build
  -Generator NAME     Specify CMake generator (e.g., "Visual Studio 17 2022")
  -UseVcpkg           Use vcpkg for dependencies
  -VcpkgRoot PATH     Path to vcpkg installation
  -SetupVcpkg         Automatically setup vcpkg and install Boost
  -Help               Show this help message

Examples:
  .\install_unilink.ps1
  .\install_unilink.ps1 -Debug -NoTests
  .\install_unilink.ps1 -Release -InstallPrefix "C:\unilink"
  .\install_unilink.ps1 -UseVcpkg -VcpkgRoot "C:\vcpkg"
  .\install_unilink.ps1 -SetupVcpkg

"@
    exit 0
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "unilink Windows Build Script (PowerShell)" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Find project root directory
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir

# Check if CMakeLists.txt exists in project root
if (-not (Test-Path "$ProjectRoot\CMakeLists.txt")) {
    Write-Host "ERROR: CMakeLists.txt not found in project root: $ProjectRoot" -ForegroundColor Red
    Write-Host "Please run this script from the unilink repository" -ForegroundColor Red
    exit 1
}

Write-Host "Project Root: $ProjectRoot" -ForegroundColor Green
Write-Host ""

# Change to project root directory
Push-Location $ProjectRoot

# Setup vcpkg if requested
if ($SetupVcpkg) {
    Write-Host "Setting up vcpkg and installing Boost..." -ForegroundColor Cyan
    Write-Host ""
    
    # Default vcpkg location
    $defaultVcpkgRoot = "C:\vcpkg"
    
    if (-not (Test-Path $defaultVcpkgRoot)) {
        Write-Host "Installing vcpkg to $defaultVcpkgRoot..." -ForegroundColor Yellow
        
        # Clone vcpkg
        Push-Location C:\
        git clone https://github.com/Microsoft/vcpkg.git
        Pop-Location
        
        if (-not (Test-Path $defaultVcpkgRoot)) {
            Write-Host "ERROR: Failed to clone vcpkg" -ForegroundColor Red
            Pop-Location
            exit 1
        }
        
        # Bootstrap vcpkg
        Push-Location $defaultVcpkgRoot
        .\bootstrap-vcpkg.bat
        Pop-Location
        
        Write-Host "vcpkg installed successfully!" -ForegroundColor Green
    } else {
        Write-Host "vcpkg already exists at $defaultVcpkgRoot" -ForegroundColor Green
    }
    
    # Install Boost
    Write-Host ""
    Write-Host "Installing Boost via vcpkg (this may take several minutes)..." -ForegroundColor Yellow
    Push-Location $defaultVcpkgRoot
    .\vcpkg install boost-system:x64-windows boost-asio:x64-windows
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Failed to install Boost" -ForegroundColor Red
        Pop-Location
        Pop-Location
        exit 1
    }
    
    .\vcpkg integrate install
    Pop-Location
    
    Write-Host ""
    Write-Host "Boost installed successfully!" -ForegroundColor Green
    Write-Host ""
    
    # Enable vcpkg usage
    $UseVcpkg = $true
    $VcpkgRoot = $defaultVcpkgRoot
}

# Determine build type
$BuildType = if ($Debug) { "Debug" } else { "Release" }
$BuildTests = if ($NoTests) { "OFF" } else { "ON" }
$BuildExamples = if ($NoExamples) { "OFF" } else { "ON" }
$BuildDir = "build"

Write-Host "Build Configuration:" -ForegroundColor Yellow
Write-Host "  Build Type: $BuildType"
Write-Host "  Build Tests: $BuildTests"
Write-Host "  Build Examples: $BuildExamples"
Write-Host "  Build Directory: $BuildDir"
Write-Host ""

# Check for CMake
$cmakePath = $null
if (Get-Command cmake -ErrorAction SilentlyContinue) {
    $cmakePath = "cmake"
} else {
    # Try to find CMake in Visual Studio installation
    Write-Host "CMake not found in PATH, searching in Visual Studio installation..." -ForegroundColor Yellow
    
    $vsCmakePaths = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    )
    
    foreach ($path in $vsCmakePaths) {
        if (Test-Path $path) {
            $cmakePath = $path
            Write-Host "Found CMake at: $cmakePath" -ForegroundColor Green
            break
        }
    }
    
    if (-not $cmakePath) {
        Write-Host ""
        Write-Host "ERROR: CMake not found" -ForegroundColor Red
        Write-Host ""
        Write-Host "Please choose one of the following options:" -ForegroundColor Yellow
        Write-Host "  1. Use 'Developer PowerShell for VS 2022' instead of regular PowerShell"
        Write-Host "  2. Install CMake from Visual Studio Installer:"
        Write-Host "     - Open Visual Studio Installer"
        Write-Host "     - Modify your installation"
        Write-Host "     - Under 'Individual Components', search for 'CMake'"
        Write-Host "     - Install 'C++ CMake tools for Windows'"
        Write-Host "  3. Install standalone CMake from: https://cmake.org/download/"
        Write-Host ""
        exit 1
    }
}

# Check for Visual Studio
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWhere) {
    $vsPath = & $vsWhere -latest -property installationPath
    Write-Host "Using Visual Studio from: $vsPath" -ForegroundColor Green
} else {
    Write-Host "WARNING: Visual Studio not detected" -ForegroundColor Yellow
}

# Prepare CMake arguments
$cmakeArgs = @(
    "-S", ".",
    "-B", $BuildDir,
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DUNILINK_BUILD_TESTS=$BuildTests",
    "-DUNILINK_BUILD_EXAMPLES=$BuildExamples",
    "-DUNILINK_BUILD_DOCS=OFF"
)

# Add generator if specified
if ($Generator) {
    $cmakeArgs += "-G", $Generator
}

# Add vcpkg toolchain if requested
if ($UseVcpkg) {
    if (-not $VcpkgRoot) {
        # Try to find vcpkg
        $vcpkgPaths = @(
            "$env:VCPKG_ROOT",
            "C:\vcpkg",
            "$env:USERPROFILE\vcpkg"
        )
        foreach ($path in $vcpkgPaths) {
            if ($path -and (Test-Path "$path\scripts\buildsystems\vcpkg.cmake")) {
                $VcpkgRoot = $path
                break
            }
        }
    }
    
    if ($VcpkgRoot -and (Test-Path "$VcpkgRoot\scripts\buildsystems\vcpkg.cmake")) {
        Write-Host "Using vcpkg from: $VcpkgRoot" -ForegroundColor Green
        $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$VcpkgRoot\scripts\buildsystems\vcpkg.cmake"
    } else {
        Write-Host "ERROR: vcpkg not found. Please specify -VcpkgRoot or set VCPKG_ROOT" -ForegroundColor Red
        exit 1
    }
}

# Configure
Write-Host ""
Write-Host "[1/3] Configuring..." -ForegroundColor Cyan
& $cmakePath @cmakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: Configuration failed" -ForegroundColor Red
    Write-Host ""
    
    # Check if it's a Boost-related error
    if ((Get-Content "$BuildDir\CMakeCache.txt" -ErrorAction SilentlyContinue) -match "Boost") {
        Write-Host "It appears Boost is not installed. Here are your options:" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "Option 1: Use vcpkg (Recommended - Easiest)" -ForegroundColor Cyan
        Write-Host "  # Install vcpkg if not already installed:" -ForegroundColor Gray
        Write-Host "  cd C:\" -ForegroundColor Gray
        Write-Host "  git clone https://github.com/Microsoft/vcpkg.git" -ForegroundColor Gray
        Write-Host "  cd vcpkg" -ForegroundColor Gray
        Write-Host "  .\bootstrap-vcpkg.bat" -ForegroundColor Gray
        Write-Host "  .\vcpkg integrate install" -ForegroundColor Gray
        Write-Host "" 
        Write-Host "  # Install Boost:" -ForegroundColor Gray
        Write-Host "  .\vcpkg install boost-system:x64-windows" -ForegroundColor Gray
        Write-Host ""
        Write-Host "  # Then run this script with vcpkg:" -ForegroundColor Gray
        Write-Host "  .\scripts\install_unilink.ps1 -UseVcpkg -VcpkgRoot `"C:\vcpkg`"" -ForegroundColor Green
        Write-Host ""
        Write-Host "Option 2: Download pre-built Boost" -ForegroundColor Cyan
        Write-Host "  Download from: https://sourceforge.net/projects/boost/files/boost-binaries/" -ForegroundColor Gray
        Write-Host "  Install and set BOOST_ROOT environment variable" -ForegroundColor Gray
        Write-Host ""
        Write-Host "Option 3: Use Conan package manager" -ForegroundColor Cyan
        Write-Host "  pip install conan" -ForegroundColor Gray
        Write-Host "  conan profile detect --force" -ForegroundColor Gray
        Write-Host "  conan install . --output-folder=build --build=missing" -ForegroundColor Gray
        Write-Host ""
    }
    
    exit 1
}

# Build
Write-Host ""
Write-Host "[2/3] Building..." -ForegroundColor Cyan
& $cmakePath --build $BuildDir --config $BuildType -j

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed" -ForegroundColor Red
    exit 1
}

# Run tests if enabled
if ($BuildTests -eq "ON") {
    Write-Host ""
    Write-Host "[3/3] Running tests..." -ForegroundColor Cyan
    Push-Location $BuildDir
    & ctest -C $BuildType --output-on-failure
    $testResult = $LASTEXITCODE
    Pop-Location
    
    if ($testResult -ne 0) {
        Write-Host "WARNING: Some tests failed" -ForegroundColor Yellow
    } else {
        Write-Host "Tests passed successfully!" -ForegroundColor Green
    }
} else {
    Write-Host "[3/3] Tests disabled, skipping..." -ForegroundColor Yellow
}

# Install if requested
if ($InstallPrefix) {
    Write-Host ""
    Write-Host "Installing to: $InstallPrefix" -ForegroundColor Cyan
    & $cmakePath --install $BuildDir --config $BuildType --prefix $InstallPrefix
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Installation failed" -ForegroundColor Red
        exit 1
    }
}

# Summary
Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

Write-Host "Build artifacts:" -ForegroundColor Yellow
Write-Host "  Library: $BuildDir\$BuildType\unilink.dll"
Write-Host "  Import lib: $BuildDir\$BuildType\unilink.lib"

if ($BuildExamples -eq "ON") {
    Write-Host ""
    Write-Host "Examples:" -ForegroundColor Yellow
    Write-Host "  $BuildDir\examples\tcp\single-echo\$BuildType\echo_tcp_server.exe"
    Write-Host "  $BuildDir\examples\tcp\single-echo\$BuildType\echo_tcp_client.exe"
}

if ($BuildTests -eq "ON") {
    Write-Host ""
    Write-Host "To run tests:" -ForegroundColor Yellow
    Write-Host "  cd $BuildDir; ctest -C $BuildType"
}

Write-Host ""

# Return to original directory
Pop-Location

exit 0

