@echo off
REM Build script for Windows deployment

echo === Multimedia Streaming App - Windows Build Script ===
echo.

REM Check if CMake is available
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found. Please install CMake and add it to PATH.
    exit /b 1
)

REM Check if we're in the project root
if not exist "CMakeLists.txt" (
    echo ERROR: CMakeLists.txt not found. Please run this script from the project root.
    exit /b 1
)

echo Checking vcpkg installation...
if not exist "C:\vcpkg\vcpkg.exe" (
    echo WARNING: vcpkg not found at C:\vcpkg\
    echo Please install SDL2 manually or adjust vcpkg path in CMakeLists.txt
    pause
)

REM Create build directory
echo Creating build directory...
if not exist "build" mkdir build
cd build

REM Configure with CMake
echo Configuring project with CMake...
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed
    cd ..
    exit /b 1
)

REM Build Release version
echo Building Release version...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed
    cd ..
    exit /b 1
)

cd ..

echo.
echo === Build Complete ===
echo Executables are in: build\Release\
echo.
echo Available programs:
echo   - screen_share.exe        : Main GUI application with streaming server
echo   - test_stream_app.exe     : Console streaming server (no GUI)
echo   - test_viewer.exe         : Streaming client viewer
echo   - test_e2e_streaming.exe  : End-to-end test
echo   - test_microphone.exe     : Audio capture test
echo.
echo To run the server: build\Release\screen_share.exe (GUI) or test_stream_app.exe (console)
echo To run the client: build\Release\test_viewer.exe
echo.
pause
