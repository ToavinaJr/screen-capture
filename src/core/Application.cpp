#include "Application.h"
#include "../utils/Logger.h"
#include "../display/SDLRenderer.h"
#include "../network/StreamServer.h"
#include "../audio/MicrophoneCapture.h"
#include "../threading/ThreadPool.h"
#include "../capture/ScreenCapture.h"
#include "common.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <memory>
#include <cstring>                 
#include <cmath>

Application::Application() 
    : window(nullptr)
    , renderer(nullptr)
    , isRunning(false)
    , streaming(false)
    , enableAudio(true)
    , audioFrameCounter(0)
    , enableStreaming(true)
    , streamPort(9999)
    , streamFps(30)
    , frameCounter(0) {
    Logger::log(Logger::LogLevel::INFO, "Application created");
}

Application::~Application() {
    shutdown();
    Logger::log(Logger::LogLevel::INFO, "Application destroyed");
}

void Application::init() {
    Logger::init("app.log");
    Logger::log(Logger::LogLevel::INFO, "Multimedia Streaming Application Starting");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        Logger::log(Logger::LogLevel::WARN, "Failed to initialize SDL");
        throw std::runtime_error("SDL initialization failed");
    }

    window = SDL_CreateWindow(
        "Screen Share",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        Logger::log(Logger::LogLevel::WARN, "Failed to create window");
        SDL_Quit();
        throw std::runtime_error("Window creation failed");
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        Logger::log(Logger::LogLevel::WARN, "Failed to create renderer");
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error("Renderer creation failed");
    }

    // Initialize streaming server
    if (enableStreaming) {
        streamServer = std::make_unique<StreamServer>("0.0.0.0", streamPort);
        if (streamServer->start()) {
            Logger::log(Logger::LogLevel::INFO, "StreamServer started on port " + std::to_string(streamPort));
            streaming = true;
            
            // Check if we're running under Wayland
            const char* wayland_display = getenv("WAYLAND_DISPLAY");
            const char* xdg_session_type = getenv("XDG_SESSION_TYPE");
            bool is_wayland = (wayland_display && wayland_display[0] != '\0') || 
                             (xdg_session_type && std::string(xdg_session_type) == "wayland");
            
            if (is_wayland) {
                Logger::log(Logger::LogLevel::WARN, 
                    "Wayland session detected. System-wide screen capture is not available. "
                    "Will capture SDL window content only. For full screen capture, please run under X11 session.");
            } else {
                screenCapture = std::make_unique<ScreenCapture>();
                if (screenCapture->init()) {
                    Logger::log(Logger::LogLevel::INFO, "Screen capture initialized");
                } else {
                    Logger::log(Logger::LogLevel::WARN, "Failed to initialize screen capture: " + screenCapture->getLastError());
                    Logger::log(Logger::LogLevel::WARN, "Will capture SDL window content only");
                    screenCapture.reset();
                }
            }
            
            // Thread will be started in run() after isRunning is set
        } else {
            Logger::log(Logger::LogLevel::ERROR_LEVEL, "Failed to start StreamServer");
        }
    }
    
    // Initialize audio capture
    if (enableAudio && streamServer) {
        microphone = std::make_unique<MicrophoneCapture>();
        
        // Start capture with callback to broadcast audio frames
        auto audioCallback = [this](const void* data, size_t size) {
            if (streamServer && streaming) {
                // Data is raw bytes (Float32 samples from SDL)
                const float* samples = static_cast<const float*>(data);
                int sampleCount = size / sizeof(float);
                
                // Create AudioFrame
                AudioFrame frame;
                frame.frame_number = audioFrameCounter++;
                frame.sample_rate = 44100;
                frame.channels = 1;
                frame.timestamp = get_timestamp_us();
                
                // Copy audio samples
                frame.samples.resize(sampleCount);
                for (int i = 0; i < sampleCount; i++) {
                    frame.samples[i] = samples[i];
                }
                
                // Broadcast to all clients
                streamServer->broadcastAudioFrame(frame);
            }
        };
        
        if (microphone->startCapture(audioCallback)) {
            Logger::log(Logger::LogLevel::INFO, "Microphone capture started");
        } else {
            Logger::log(Logger::LogLevel::WARN, "Failed to start microphone capture (device may not be available)");
            microphone.reset();
        }
    }

    Logger::log(Logger::LogLevel::INFO, "Application initialized successfully");
}

void Application::run() {
    if (isRunning) {
        Logger::log(Logger::LogLevel::WARN, "Application already running");
        return;
    }

    isRunning = true;
    Logger::log(Logger::LogLevel::INFO, "Application running...");

    // Start capture and stream thread now that isRunning is true
    if (streaming && streamServer && !streamThread.joinable()) {
        streamThread = std::thread(&Application::captureAndStream, this);
    }

    mainLoop();
}

void Application::shutdown() {
    if (!isRunning) {
        return;
    }

    isRunning = false;
    Logger::log(Logger::LogLevel::INFO, "Application shutting down...");

    // Stop audio capture
    if (microphone) {
        microphone->stopCapture();
        microphone.reset();
    }

    // Stop streaming
    if (streaming) {
        streaming = false;
        if (streamThread.joinable()) {
            streamThread.join();
        }
    }
    
    if (streamServer) {
        streamServer->stop();
        streamServer.reset();
    }

    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();

    Logger::shutdown();
}

void Application::mainLoop() {
    SDL_Event event;
    bool quit = false;

    Logger::log(Logger::LogLevel::INFO, "Entering main loop");

    while (!quit && isRunning) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
                Logger::log(Logger::LogLevel::INFO, "Quit event received");
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    quit = true;
                    Logger::log(Logger::LogLevel::INFO, "Escape key pressed, quitting");
                }
            }
        }

        handleEvents();
        render();

        SDL_Delay(16);
    }

    Logger::log(Logger::LogLevel::INFO, "Exiting main loop - quit=" + std::string(quit ? "true" : "false") + 
                ", isRunning=" + std::string(isRunning ? "true" : "false"));
    isRunning = false;
}

void Application::handleEvents() {
    // Additional event handling logic
}

void Application::render() {
    std::lock_guard<std::mutex> lock(renderer_mutex_);
    if (!renderer) return;
    
    // Clear with dark background
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderClear(renderer);
    
    // Draw some content for testing
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    
    // Animated rectangle
    int time = SDL_GetTicks();
    int x = (int)((sin(time / 1000.0) * 0.5 + 0.5) * (w - 200));
    int y = (int)((cos(time / 800.0) * 0.5 + 0.5) * (h - 200));
    
    SDL_Rect rect = {x, y, 200, 150};
    SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
    SDL_RenderFillRect(renderer, &rect);
    
    // Draw border
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(renderer, &rect);
    
    // Draw some lines
    for (int i = 0; i < 10; i++) {
        int hue = (time / 10 + i * 36) % 360;
        SDL_SetRenderDrawColor(renderer, 
            (Uint8)(sin(hue * 0.0174533) * 127 + 128),
            (Uint8)(sin((hue + 120) * 0.0174533) * 127 + 128),
            (Uint8)(sin((hue + 240) * 0.0174533) * 127 + 128),
            255);
        SDL_RenderDrawLine(renderer, w/2, h/2, 
            (int)(w/2 + cos(time / 500.0 + i * 0.628) * 300),
            (int)(h/2 + sin(time / 500.0 + i * 0.628) * 300));
    }
    
    SDL_RenderPresent(renderer);
}

std::vector<uint8_t> Application::captureFrame() {
    // Use ScreenCapture if available, otherwise fallback to SDL renderer capture
    if (screenCapture && screenCapture->isInitialized()) {
        int width, height;
        std::vector<uint8_t> pixels = screenCapture->captureScreen(width, height);
        
        if (!pixels.empty()) {
            return pixels;
        } else {
            Logger::log(Logger::LogLevel::WARN, "Screen capture failed: " + screenCapture->getLastError());
        }
    }
    
    // Fallback to SDL renderer capture (only captures SDL window content)
    std::lock_guard<std::mutex> lock(renderer_mutex_);
    if (!renderer) {
        return std::vector<uint8_t>();
    }
    
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    
    // Create surface to read pixels
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32,
                                                  0x00FF0000,
                                                  0x0000FF00,
                                                  0x000000FF,
                                                  0xFF000000);
    if (!surface) {
        Logger::log(Logger::LogLevel::ERROR_LEVEL, "Failed to create surface for capture");
        return std::vector<uint8_t>();
    }
    
    // Read pixels from renderer
    if (SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888,
                            surface->pixels, surface->pitch) != 0) {
        Logger::log(Logger::LogLevel::ERROR_LEVEL, "Failed to read pixels: " + std::string(SDL_GetError()));
        SDL_FreeSurface(surface);
        return std::vector<uint8_t>();
    }
    
    // Copy pixel data
    int dataSize = surface->pitch * height;
    std::vector<uint8_t> pixels(dataSize);
    memcpy(pixels.data(), surface->pixels, dataSize);
    
    SDL_FreeSurface(surface);
    return pixels;
}

void Application::captureAndStream() {
    Logger::log(Logger::LogLevel::INFO, "Capture and stream thread started");
    
    const int targetFrameTime = 1000 / streamFps; // ms per frame
    uint32_t lastFrameTime = SDL_GetTicks();
    uint32_t localFrameCounter = 0;
    
    while (streaming && isRunning) {
        uint32_t currentTime = SDL_GetTicks();
        uint32_t elapsed = currentTime - lastFrameTime;
        
        if (elapsed >= targetFrameTime) {
            lastFrameTime = currentTime;
            
            // Capture frame
            std::vector<uint8_t> frameData = captureFrame();
            
            if (!frameData.empty() && streamServer) {
                int width, height;
                SDL_GetWindowSize(window, &width, &height);
                
                // Create VideoFrame
                VideoFrame frame;
                frame.frame_number = localFrameCounter++;
                frame.width = width;
                frame.height = height;
                frame.quality = 80;
                frame.timestamp = get_timestamp_us();
                frame.data = std::move(frameData);
                
                // Broadcast to all clients
                streamServer->broadcastVideoFrame(frame);
                
                // Log every 30 frames
                if (localFrameCounter % 30 == 0) {
                    size_t clientCount = streamServer->getClientCount();
                    std::string audioStatus = microphone ? " (audio: ON)" : " (audio: OFF)";
                    Logger::log(Logger::LogLevel::INFO, 
                        "Streamed video frame " + std::to_string(localFrameCounter) + 
                        " to " + std::to_string(clientCount) + " client(s)" + audioStatus);
                }
            }
        }
        
        // Sleep to avoid busy waiting
        SDL_Delay(5);
    }
    
    Logger::log(Logger::LogLevel::INFO, "Capture and stream thread ended");
}
