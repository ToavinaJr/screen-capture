#ifndef APPLICATION_H
#define APPLICATION_H

#include "Config.h"
#include <SDL2/SDL.h>
#include <thread>
#include <atomic>
#include <memory>
#include <vector>
#include <mutex>

class StreamServer;
class MicrophoneCapture;
class ScreenCapture;

class Application {
public:
    Application();
    ~Application();

    void init();
    void run();
    void shutdown();

private:
    void mainLoop();
    void handleEvents();
    void render();
    void captureAndStream();
    std::vector<uint8_t> captureFrame();
    
    SDL_Window* window;
    SDL_Renderer* renderer;
    std::mutex renderer_mutex_;
    std::atomic<bool> isRunning;
    std::thread renderThread;
    
    // Streaming components
    std::unique_ptr<StreamServer> streamServer;
    std::unique_ptr<ScreenCapture> screenCapture;
    std::thread streamThread;
    std::atomic<bool> streaming;
    
    // Audio components
    std::unique_ptr<MicrophoneCapture> microphone;
    bool enableAudio;
    uint32_t audioFrameCounter;
    
    // Configuration
    bool enableStreaming;
    int streamPort;
    int streamFps;
    uint32_t frameCounter;
};

#endif // APPLICATION_H