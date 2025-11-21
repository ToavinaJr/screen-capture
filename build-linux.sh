#!/bin/bash
# Build script for Linux deployment

set -e  # Exit on error

echo "=== Multimedia Streaming App - Linux Build Script ==="
echo ""

# Check prerequisites
echo "Checking prerequisites..."
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found. Please install cmake."
    exit 1
fi

if ! command -v g++ &> /dev/null; then
    echo "ERROR: g++ not found. Please install build-essential."
    exit 1
fi

# Check SDL2
if ! pkg-config --exists sdl2; then
    echo "WARNING: SDL2 not found. Installing dependencies..."
    
    if [ -f /etc/debian_version ]; then
        sudo apt-get update
        sudo apt-get install -y libsdl2-dev libssl-dev pkg-config
    elif [ -f /etc/redhat-release ]; then
        sudo dnf install -y SDL2-devel openssl-devel
    elif [ -f /etc/arch-release ]; then
        sudo pacman -S sdl2 openssl
    else
        echo "ERROR: Unsupported distribution. Please install SDL2 and OpenSSL manually."
        exit 1
    fi
fi

echo "All prerequisites OK!"
echo ""

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Configure
echo "Configuring project with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON

# Build
echo "Building project..."
cmake --build . -j$(nproc)

echo ""
echo "=== Build Complete ==="
echo "Executables are in: $(pwd)/Release/ or $(pwd)/"
echo ""
echo "Available programs:"
echo "  - screen_share          : Main GUI application with streaming server"
echo "  - test_stream_app       : Console streaming server (no GUI)"
echo "  - test_viewer          : Streaming client viewer"
echo "  - test_e2e_streaming   : End-to-end test"
echo "  - test_microphone      : Audio capture test"
echo ""
echo "To run the server: ./screen_share (GUI) or ./test_stream_app (console)"
echo "To run the client: ./test_viewer"
