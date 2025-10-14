@echo off
REM Windows Build Script for unilink
REM Copyright 2025 Jinwoo Sung

setlocal enabledelayedexpansion

echo ========================================
echo unilink Windows Build Script
echo ========================================
echo.

REM Find project root directory
set SCRIPT_DIR=%~dp0
cd /d "%SCRIPT_DIR%.."
set PROJECT_ROOT=%CD%

REM Check if CMakeLists.txt exists in project root
if not exist "%PROJECT_ROOT%\CMakeLists.txt" (
    echo ERROR: CMakeLists.txt not found in project root: %PROJECT_ROOT%
    echo Please run this script from the unilink repository
    exit /b 1
)

echo Project Root: %PROJECT_ROOT%
echo.

REM Default build type
set BUILD_TYPE=Release
set BUILD_TESTS=ON
set BUILD_EXAMPLES=ON
set BUILD_DIR=build
set GENERATOR=
set INSTALL_PREFIX=

REM Parse command line arguments
:parse_args
if "%1"=="" goto end_parse_args
if /i "%1"=="--debug" (
    set BUILD_TYPE=Debug
    shift
    goto parse_args
)
if /i "%1"=="--release" (
    set BUILD_TYPE=Release
    shift
    goto parse_args
)
if /i "%1"=="--no-tests" (
    set BUILD_TESTS=OFF
    shift
    goto parse_args
)
if /i "%1"=="--no-examples" (
    set BUILD_EXAMPLES=OFF
    shift
    goto parse_args
)
if /i "%1"=="--install" (
    set INSTALL_PREFIX=%2
    shift
    shift
    goto parse_args
)
if /i "%1"=="--help" (
    goto show_help
)
shift
goto parse_args
:end_parse_args

echo Build Configuration:
echo   Build Type: %BUILD_TYPE%
echo   Build Tests: %BUILD_TESTS%
echo   Build Examples: %BUILD_EXAMPLES%
echo   Build Directory: %BUILD_DIR%
echo.

REM Check for CMake
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake or use Developer Command Prompt
    exit /b 1
)

REM Detect Visual Studio version
if defined VSINSTALLDIR (
    echo Using Visual Studio from: %VSINSTALLDIR%
) else (
    echo WARNING: Visual Studio environment not detected
    echo Please run from Developer Command Prompt or Visual Studio Command Prompt
)

REM Configure
echo.
echo [1/3] Configuring...
cmake -S . -B %BUILD_DIR% ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DUNILINK_BUILD_TESTS=%BUILD_TESTS% ^
    -DUNILINK_BUILD_EXAMPLES=%BUILD_EXAMPLES% ^
    -DUNILINK_BUILD_DOCS=OFF

if %errorlevel% neq 0 (
    echo ERROR: Configuration failed
    exit /b 1
)

REM Build
echo.
echo [2/3] Building...
cmake --build %BUILD_DIR% --config %BUILD_TYPE% -j

if %errorlevel% neq 0 (
    echo ERROR: Build failed
    exit /b 1
)

REM Run tests if enabled
if "%BUILD_TESTS%"=="ON" (
    echo.
    echo [3/3] Running tests...
    cd %BUILD_DIR%
    ctest -C %BUILD_TYPE% --output-on-failure
    cd ..
    
    if %errorlevel% neq 0 (
        echo WARNING: Some tests failed
    ) else (
        echo Tests passed successfully!
    )
) else (
    echo [3/3] Tests disabled, skipping...
)

REM Install if requested
if not "%INSTALL_PREFIX%"=="" (
    echo.
    echo Installing to: %INSTALL_PREFIX%
    cmake --install %BUILD_DIR% --config %BUILD_TYPE% --prefix "%INSTALL_PREFIX%"
    
    if %errorlevel% neq 0 (
        echo ERROR: Installation failed
        exit /b 1
    )
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Build artifacts:
if "%BUILD_TYPE%"=="Debug" (
    echo   Library: %BUILD_DIR%\Debug\unilink.dll
    echo   Import lib: %BUILD_DIR%\Debug\unilink.lib
) else (
    echo   Library: %BUILD_DIR%\Release\unilink.dll
    echo   Import lib: %BUILD_DIR%\Release\unilink.lib
)

if "%BUILD_EXAMPLES%"=="ON" (
    echo.
    echo Examples:
    echo   %BUILD_DIR%\examples\tcp\single-echo\%BUILD_TYPE%\echo_tcp_server.exe
    echo   %BUILD_DIR%\examples\tcp\single-echo\%BUILD_TYPE%\echo_tcp_client.exe
)

if "%BUILD_TESTS%"=="ON" (
    echo.
    echo To run tests:
    echo   cd %BUILD_DIR% ^&^& ctest -C %BUILD_TYPE%
)

echo.
exit /b 0

:show_help
echo Usage: build_windows.bat [options]
echo.
echo Options:
echo   --debug           Build in Debug mode (default: Release)
echo   --release         Build in Release mode
echo   --no-tests        Disable building tests
echo   --no-examples     Disable building examples
echo   --install PATH    Install to specified path after build
echo   --help            Show this help message
echo.
echo Examples:
echo   build_windows.bat
echo   build_windows.bat --debug --no-tests
echo   build_windows.bat --release --install C:\unilink
echo.
exit /b 0

