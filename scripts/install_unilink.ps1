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
  -Help               Show this help message

Examples:
  .\build_windows.ps1
  .\build_windows.ps1 -Debug -NoTests
  .\build_windows.ps1 -Release -InstallPrefix "C:\unilink"
  .\build_windows.ps1 -UseVcpkg -VcpkgRoot "C:\vcpkg"

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
    Write-Host "ERROR: Configuration failed" -ForegroundColor Red
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

