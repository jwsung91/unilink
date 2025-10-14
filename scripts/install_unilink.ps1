Param(
    [string]$BuildDir = "build-windows",
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Configuration = "Release",
    [switch]$BuildTests,
    [switch]$BuildExamples,
    [switch]$Install,
    [string]$Generator = ""
)

$ErrorActionPreference = "Stop"

function Resolve-Generator {
    param ([string]$Requested)

    if ($Requested) {
        return $Requested
    }

    # Prefer Ninja if available, fall back to Visual Studio
    if (Get-Command ninja -ErrorAction SilentlyContinue) {
        return "Ninja"
    }

    return "Visual Studio 17 2022"
}

$RootDir = Resolve-Path (Join-Path $PSScriptRoot "..")
$BuildPath = Join-Path $RootDir $BuildDir
$ResolvedGenerator = Resolve-Generator -Requested $Generator

if (-not (Test-Path $BuildPath)) {
    New-Item -ItemType Directory -Path $BuildPath | Out-Null
}

$cmakeConfigureArgs = @(
    "-S", $RootDir
    "-B", $BuildPath
    "-DUNILINK_BUILD_SHARED=ON"
    "-DUNILINK_BUILD_STATIC=OFF"
    "-DUNILINK_BUILD_TESTS=" + ($BuildTests.IsPresent ? "ON" : "OFF")
    "-DUNILINK_BUILD_EXAMPLES=" + ($BuildExamples.IsPresent ? "ON" : "OFF")
    "-DUNILINK_ENABLE_INSTALL=ON"
    "-DUNILINK_ENABLE_PKGCONFIG=ON"
)

if ($ResolvedGenerator) {
    $cmakeConfigureArgs += @("-G", $ResolvedGenerator)
}

Write-Host "Configuring Unilink with generator '$ResolvedGenerator'..."
cmake @cmakeConfigureArgs

Write-Host "Building Unilink ($Configuration)..."
cmake --build $BuildPath --config $Configuration

if ($BuildTests.IsPresent) {
    Write-Host "Running unit tests..."
    cmake --build $BuildPath --config $Configuration --target RUN_TESTS
}

if ($Install.IsPresent) {
    $installArgs = @("--install", $BuildPath)
    if ($Configuration -and $ResolvedGenerator -like "Visual Studio*") {
        $installArgs += @("--config", $Configuration)
    }
    Write-Host "Installing Unilink..."
    cmake @installArgs
}

Write-Host "Completed."
