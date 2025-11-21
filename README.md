# Multimedia Streaming Application

## Overview
This project is a multimedia streaming application that captures audio from the microphone, encrypts the stream using TLS/SSL, and displays the output in real-time using SDL2. The application is designed to be efficient and responsive, utilizing multi-threading for optimal performance.

## Features
- Real-time audio capture and streaming from the microphone (with user consent).
- Secure communication using TLS/SSL encryption.
- Real-time graphics rendering using SDL2.
- Multi-threading support for improved performance and responsiveness.
- Modular architecture with separate components for audio, network, display, and threading.

## Project Structure
```
multimedia-streaming-app
├── src
│   ├── main.cpp                # Entry point of the application
│   ├── core
│   │   ├── Application.cpp      # Application lifecycle management
│   │   ├── Application.h        # Application class declaration
│   │   └── Config.h            # Configuration constants and structures
│   ├── display
│   │   ├── SDLRenderer.cpp      # SDL2 rendering implementation
│   │   ├── SDLRenderer.h        # SDLRenderer class declaration
│   │   └── DisplayManager.h     # Display management
│   ├── audio
│   │   ├── MicrophoneCapture.cpp # Microphone input handling
│   │   ├── MicrophoneCapture.h   # MicrophoneCapture class declaration
│   │   └── AudioStream.h        # Audio data management
│   ├── network
│   │   ├── TLSConnection.cpp     # TLS/SSL connection handling
│   │   ├── TLSConnection.h       # TLSConnection class declaration
│   │   ├── StreamServer.cpp      # Stream server implementation
│   │   └── StreamServer.h        # StreamServer class declaration
│   ├── threading
│   │   ├── ThreadPool.cpp        # Thread pool management
│   │   ├── ThreadPool.h          # ThreadPool class declaration
│   │   └── SafeQueue.h           # Thread-safe queue definition
│   └── utils
│       ├── Logger.cpp            # Logging functionality
│       └── Logger.h              # Logger class declaration
├── include
│   └── common.h                 # Common definitions and includes
├── tests
│   ├── test_audio.cpp           # Unit tests for audio functionality
│   ├── test_network.cpp         # Unit tests for network functionality
│   └── test_threading.cpp       # Unit tests for threading functionality
├── CMakeLists.txt               # CMake configuration file
├── cmake
│   └── FindSDL2.cmake           # CMake module for SDL2
└── README.md                    # Project documentation
```

## Build Instructions
1. Ensure you have CMake and SDL2 installed on your system.
2. Clone the repository and navigate to the project directory.
3. Create a build directory:
   ```
   mkdir build
   cd build
   ```
4. Run CMake to configure the project:
   ```
   cmake ..
   ```
5. Build the project:
   ```
   make
   ```

## Usage
- Run the application from the build directory.
- Grant microphone access when prompted.
- Enjoy real-time audio streaming with secure connections.

## Contributing
Contributions are welcome! Please submit a pull request or open an issue for any enhancements or bug fixes.

## License
This project is licensed under the MIT License. See the LICENSE file for details.