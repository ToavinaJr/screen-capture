# Multimedia Streaming Application - Release Package

## Quick Start

### Windows
1. **Server (with GUI):** Double-click `screen_share.exe`
2. **Server (console):** Double-click `test_stream_app.exe`
3. **Client:** Double-click `test_viewer.exe`

### Linux
1. **Server (with GUI):** `./screen_share`
2. **Server (console):** `./test_stream_app`
3. **Client:** `./test_viewer`

## What's Included

### Main Applications
- **screen_share** - Streaming server with SDL2 GUI window
  - Opens 1280x720 window with animated graphics
  - Captures screen at 30 FPS
  - Captures microphone audio (if available)
  - Streams to port 9999
  
- **test_stream_app** - Streaming server without GUI
  - No window required (headless)
  - Simulated video frames (30 FPS)
  - Simulated audio tone at 440Hz (10 Hz)
  - Perfect for testing or remote servers
  - Runs for 60 seconds

- **test_viewer** - Streaming client
  - Connects to streaming server
  - Displays real-time statistics:
    - Video frames received
    - Audio frames received
    - Current FPS
    - Data bandwidth
    - Video resolution

### Test Utilities
- **test_e2e_streaming** - Full end-to-end test (server + client)
- **test_microphone** - Audio capture test
- **test_logger** - Logging system test
- **test_threadpool** - Thread pool test
- **test_common** - Common utilities test

## Usage

### Local Testing (Same Computer)
1. Open terminal/command prompt #1:
   ```
   ./test_stream_app
   ```
   (or `test_stream_app.exe` on Windows)

2. Open terminal/command prompt #2:
   ```
   ./test_viewer
   ```
   (or `test_viewer.exe` on Windows)

3. Watch the streaming statistics in viewer window

### Network Testing (Different Computers)

**On Server Computer:**
1. Note your IP address:
   - Windows: Open cmd, type `ipconfig`
   - Linux: Open terminal, type `ip addr` or `ifconfig`
   
2. Open firewall port 9999:
   - Windows: `netsh advfirewall firewall add rule name="StreamServer" dir=in action=allow protocol=TCP localport=9999`
   - Linux: `sudo ufw allow 9999/tcp`

3. Run server:
   ```
   ./test_stream_app
   ```

**On Client Computer:**
1. Edit test_viewer source (line ~30) to change IP from "127.0.0.1" to server IP
2. Rebuild or use pre-configured version
3. Run viewer:
   ```
   ./test_viewer
   ```

## Protocol Details

- **Port:** 9999 (TCP)
- **Video:** 30 FPS, ~137 KB/frame
- **Audio:** 10 Hz, ~16 KB/frame
- **Bandwidth:** ~4.3 MB/s per client
- **Packet Types:** HANDSHAKE, VIDEO_FRAME, AUDIO_FRAME, DISCONNECT, CONFIG, HEARTBEAT, ACK

## System Requirements

### Windows
- Windows 10/11 (64-bit)
- Visual C++ Redistributable (usually pre-installed)
- SDL2.dll (included in package)

### Linux
- Ubuntu 18.04+ / Debian 10+ / Fedora 30+ / Arch Linux
- SDL2 library: `sudo apt-get install libsdl2-2.0-0` (Ubuntu/Debian)
- OpenSSL library: `sudo apt-get install libssl3` (Ubuntu/Debian)

## Performance

- **Single client:** ~4.3 MB/s bandwidth
- **10 clients:** ~43 MB/s bandwidth
- **CPU usage:** Low (optimized Release build)
- **Latency:** 10-50ms on LAN, 50-300ms on WAN

## Troubleshooting

**"Cannot connect to server"**
- Check server is running
- Check firewall allows port 9999
- Verify IP address is correct

**"SDL2.dll not found" (Windows)**
- Ensure SDL2.dll is in same folder as .exe files

**"No display" (Linux GUI apps)**
- Set display: `export DISPLAY=:0`
- Or use console version: `test_stream_app`

**High latency/choppy video**
- Check network bandwidth
- Reduce number of connected clients
- Use wired connection instead of WiFi

## Technical Support

For issues or questions, refer to:
- DEPLOYMENT.md - Complete deployment guide
- README.md - Project overview
- Source code in src/ directory

## License

See LICENSE file in project root.
