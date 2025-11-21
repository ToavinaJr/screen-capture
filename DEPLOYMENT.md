# Deployment Guide - Multimedia Streaming Application

## Windows Deployment

### Prerequisites
- Windows 10/11 (x64)
- Visual C++ Redistributable 2019-2022 (usually pre-installed)

### Files to Deploy
All executables are located in `build/Release/`:

**Main Application:**
- `screen_share.exe` - Main application with SDL2 window, screen capture, and streaming server
- `SDL2.dll` - Required SDL2 library (included)

**Test/Utility Programs:**
- `test_stream_app.exe` - Console streaming server with simulated video/audio (no SDL window)
- `test_viewer.exe` - Client viewer showing real-time streaming statistics
- `test_e2e_streaming.exe` - End-to-end streaming test (server + client)
- `test_microphone.exe` - Audio capture test
- `test_logger.exe` - Logger test
- `test_threadpool.exe` - Thread pool test
- `test_common.exe` - Common utilities test

### Windows Usage

**Running the Server (with GUI):**
```cmd
screen_share.exe
```
- Opens SDL2 window (1280x720)
- Starts streaming server on port 9999
- Captures screen at 30 FPS
- Captures microphone audio (if available)

**Running the Server (console only):**
```cmd
test_stream_app.exe
```
- No GUI required
- Streaming server on port 9999
- Simulated video (30 FPS) + audio (440Hz tone, 10 Hz)
- Runs for 60 seconds

**Running the Client:**
```cmd
test_viewer.exe
```
- Connects to 127.0.0.1:9999 (localhost)
- Displays real-time statistics (FPS, data received, frame counts)

### Port Configuration
- Default port: **9999**
- Protocol: TCP
- Firewall: Open port 9999 for remote connections

---

## Linux Deployment

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libsdl2-dev \
    libssl-dev \
    pkg-config

# Fedora/RHEL
sudo dnf install -y \
    gcc-c++ \
    cmake \
    SDL2-devel \
    openssl-devel

# Arch Linux
sudo pacman -S \
    base-devel \
    cmake \
    sdl2 \
    openssl
```

### Building on Linux

1. **Clone/Copy the project:**
```bash
cd /path/to/multimedia-streaming-app
```

2. **Create build directory:**
```bash
mkdir -p build && cd build
```

3. **Configure with CMake:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

4. **Build:**
```bash
cmake --build . -j$(nproc)
```

5. **Executables location:**
```bash
ls -la Release/
# or directly in build/ depending on generator
```

### Linux Usage

**Running the Server (with GUI):**
```bash
./Release/screen_share
# or
./screen_share
```
- Requires X11/Wayland display
- SDL2 window with rendering
- Streaming server on port 9999

**Running the Server (console only):**
```bash
./Release/test_stream_app
# or
./test_stream_app
```
- No display required
- Can run on headless servers

**Running the Client:**
```bash
./Release/test_viewer
# or
./test_viewer
```

---

## Cross-Platform Testing

### Network Configuration

**Server Machine:**
1. Note the IP address:
   - Windows: `ipconfig`
   - Linux: `ip addr` or `ifconfig`

2. Open firewall port 9999:
   - Windows: `netsh advfirewall firewall add rule name="StreamServer" dir=in action=allow protocol=TCP localport=9999`
   - Linux: `sudo ufw allow 9999/tcp` (UFW) or `sudo firewall-cmd --add-port=9999/tcp --permanent` (firewalld)

**Client Machine:**
Edit `test_viewer.cpp` before building to change connection address:
```cpp
// Line ~30: Change from localhost to server IP
if (!client.connect("192.168.1.100", 9999)) {  // Replace with server IP
```

### Performance Testing

**Bandwidth Estimation:**
- Video: ~137 KB/frame at 30 FPS = ~4.1 MB/s
- Audio: ~16 KB/frame at 10 Hz = ~160 KB/s
- Total: ~4.3 MB/s per client
- For 10 clients: ~43 MB/s bandwidth required

**Latency:**
- Local network (LAN): 10-50ms
- WAN/Internet: 50-300ms depending on connection

---

## Package Structure for Distribution

### Windows Portable Package
```
StreamApp-Windows-x64/
├── screen_share.exe
├── test_stream_app.exe
├── test_viewer.exe
├── SDL2.dll
├── README.txt
└── vcredist_x64.exe (optional)
```

### Linux Package
```
StreamApp-Linux-x64/
├── screen_share
├── test_stream_app
├── test_viewer
├── README.txt
└── install-dependencies.sh
```

**install-dependencies.sh:**
```bash
#!/bin/bash
if [ -f /etc/debian_version ]; then
    sudo apt-get install -y libsdl2-2.0-0 libssl3
elif [ -f /etc/redhat-release ]; then
    sudo dnf install -y SDL2 openssl-libs
elif [ -f /etc/arch-release ]; then
    sudo pacman -S sdl2 openssl
fi
```

---

## Protocol Specification

**Port:** 9999 (TCP)

**Packet Types:**
1. HANDSHAKE (0) - Initial connection
2. VIDEO_FRAME (1) - Screen capture data
3. AUDIO_FRAME (2) - Microphone data
4. DISCONNECT (3) - Connection termination
5. CONFIG (4) - Configuration changes
6. HEARTBEAT (5) - Keep-alive (every 10s)
7. ACK (6) - Acknowledgment

**Packet Header (24 bytes):**
- magic: 0x5343524E
- version: 1
- packet_type: 0-6
- flags: 0
- payload_size: variable
- sequence_number: auto-increment
- timestamp: milliseconds

---

## Troubleshooting

**Windows:**
- "SDL2.dll not found" → Copy SDL2.dll to same folder as .exe
- "Port already in use" → Kill process: `netstat -ano | findstr :9999`, then `taskkill /PID <pid> /F`

**Linux:**
- "libSDL2 not found" → Install SDL2: `sudo apt-get install libsdl2-2.0-0`
- "Permission denied on port 9999" → Use ports > 1024 or run with sudo (not recommended)
- "No display" for GUI apps → Set `export DISPLAY=:0` or use `test_stream_app` (console only)

**Network:**
- Connection refused → Check firewall and server is running
- High latency → Check network bandwidth and reduce video quality if needed
- Packet loss → Protocol currently doesn't handle retransmission (future improvement)

---

## Development Build

For developers wanting to modify the code:

**Windows:**
```cmd
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build . --config Debug
```

**Linux:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build . -j$(nproc)
```

Debug builds include:
- Full symbol information
- No optimizations
- Detailed logging
- Assertions enabled
