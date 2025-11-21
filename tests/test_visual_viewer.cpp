#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>
#include <SDL.h>
#include "../src/network/StreamClient.h"
#include "../src/utils/Logger.h"

class VisualViewer {
public:
    VisualViewer() : window_(nullptr), renderer_(nullptr), texture_(nullptr),
                     current_width_(0), current_height_(0),
                     video_frames_(0), audio_frames_(0), running_(true) {
        start_time_ = std::chrono::steady_clock::now();
    }
    
    ~VisualViewer() {
        cleanup();
    }
    
    bool init() {
        Logger::log(Logger::LogLevel::INFO, "Initializing SDL...");
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::string error = "SDL_Init failed: " + std::string(SDL_GetError());
            std::cerr << error << std::endl;
            Logger::log(Logger::LogLevel::ERROR_LEVEL, error);
            return false;
        }
        
        Logger::log(Logger::LogLevel::INFO, "Creating SDL window...");
        window_ = SDL_CreateWindow(
            "Stream Viewer",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            1280, 720,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );
        
        if (!window_) {
            std::string error = "SDL_CreateWindow failed: " + std::string(SDL_GetError());
            std::cerr << error << std::endl;
            Logger::log(Logger::LogLevel::ERROR_LEVEL, error);
            return false;
        }
        
        Logger::log(Logger::LogLevel::INFO, "Creating SDL renderer...");
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer_) {
            std::string error = "SDL_CreateRenderer failed: " + std::string(SDL_GetError());
            std::cerr << error << std::endl;
            Logger::log(Logger::LogLevel::ERROR_LEVEL, error);
            return false;
        }
        
        Logger::log(Logger::LogLevel::INFO, "SDL initialized successfully");
        return true;
    }
    
    void onVideoFrame(const VideoFrame& frame) {
        video_frames_++;
        
        // Debug log first frame and every 30 frames
        if (video_frames_ == 1 || video_frames_ % 30 == 0) {
            std::stringstream ss;
            ss << "Received frame " << video_frames_ << " - " 
               << frame.width << "x" << frame.height 
               << ", data size: " << frame.data.size() << " bytes";
            Logger::log(Logger::LogLevel::INFO, ss.str());
            std::cout << ss.str() << std::endl;
        }
        
        // Update texture if size changed
        if (frame.width != current_width_ || frame.height != current_height_) {
            if (texture_) {
                SDL_DestroyTexture(texture_);
            }
            
            // Try BGRA8888 format which is more compatible with Windows ARGB data
            texture_ = SDL_CreateTexture(
                renderer_,
                SDL_PIXELFORMAT_BGRA8888,
                SDL_TEXTUREACCESS_STREAMING,
                frame.width,
                frame.height
            );
            
            if (!texture_) {
                Logger::log(Logger::LogLevel::ERROR_LEVEL, "Failed to create texture: " + std::string(SDL_GetError()));
                return;
            }
            
            current_width_ = frame.width;
            current_height_ = frame.height;
            
            std::stringstream ss;
            ss << "Video resolution: " << frame.width << "x" << frame.height;
            Logger::log(Logger::LogLevel::INFO, ss.str());
        }
        
        if (texture_ && !frame.data.empty()) {
            // Verify data size
            size_t expected_size = frame.width * frame.height * 4; // ARGB = 4 bytes per pixel
            if (frame.data.size() >= expected_size) {
                // Debug: Check first few pixels to see if data is valid
                if (video_frames_ == 1) {
                    std::stringstream ss;
                    ss << "First 16 bytes of pixel data: ";
                    for (int i = 0; i < 16 && i < frame.data.size(); i++) {
                        ss << std::hex << std::setfill('0') << std::setw(2) << (int)frame.data[i] << " ";
                    }
                    Logger::log(Logger::LogLevel::INFO, ss.str());
                }
                
                // Update texture with frame data
                int result = SDL_UpdateTexture(texture_, nullptr, frame.data.data(), frame.width * 4);
                if (result != 0) {
                    Logger::log(Logger::LogLevel::ERROR_LEVEL, "SDL_UpdateTexture failed: " + std::string(SDL_GetError()));
                    return;
                }
                
                // Clear with a test color to verify rendering works
                SDL_SetRenderDrawColor(renderer_, 50, 50, 50, 255);
                SDL_RenderClear(renderer_);
                
                // Render the texture
                int copy_result = SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
                if (copy_result != 0) {
                    Logger::log(Logger::LogLevel::ERROR_LEVEL, "SDL_RenderCopy failed: " + std::string(SDL_GetError()));
                } else if (video_frames_ == 1) {
                    Logger::log(Logger::LogLevel::INFO, "First frame rendered successfully");
                }
                
                // Draw FPS overlay
                drawStats();
                
                SDL_RenderPresent(renderer_);
            } else {
                std::stringstream ss;
                ss << "Frame data size mismatch: got " << frame.data.size() 
                   << " bytes, expected " << expected_size;
                Logger::log(Logger::LogLevel::WARN, ss.str());
            }
        } else if (!texture_) {
            Logger::log(Logger::LogLevel::WARN, "No texture available");
        } else if (frame.data.empty()) {
            Logger::log(Logger::LogLevel::WARN, "Frame data is empty");
        }
    }
    
    void onAudioFrame(const AudioFrame& frame) {
        audio_frames_++;
    }
    
    void onDisconnect() {
        Logger::log(Logger::LogLevel::INFO, "Disconnected from server");
        running_ = false;
    }
    
    void handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running_ = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running_ = false;
                }
            }
        }
    }
    
    bool isRunning() const { return running_; }
    
    void printStats() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();
        
        std::cout << "\n=== Final Statistics ===\n"
                  << "Total time: " << elapsed << " seconds\n"
                  << "Total video frames: " << video_frames_ << "\n"
                  << "Total audio frames: " << audio_frames_ << "\n"
                  << "Average video FPS: " << (elapsed > 0 ? video_frames_ / elapsed : 0) << "\n"
                  << "Average audio FPS: " << (elapsed > 0 ? audio_frames_ / elapsed : 0) << "\n";
    }
    
private:
    void drawStats() {
        // Calculate FPS
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();
        double fps = elapsed > 0 ? static_cast<double>(video_frames_) / elapsed : 0.0;
        
        // Draw semi-transparent background for stats
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 180);
        SDL_Rect statsRect = {10, 10, 250, 80};
        SDL_RenderFillRect(renderer_, &statsRect);
        
        // Note: For text rendering, you'd need SDL_ttf library
        // For now, we'll just show the stats in console periodically
        static auto last_print = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_print).count() >= 2) {
            std::cout << "\rFPS: " << std::fixed << std::setprecision(1) << fps
                      << " | Video: " << video_frames_
                      << " | Audio: " << audio_frames_
                      << " | Resolution: " << current_width_ << "x" << current_height_
                      << std::flush;
            last_print = now;
        }
    }
    
    void cleanup() {
        if (texture_) {
            SDL_DestroyTexture(texture_);
            texture_ = nullptr;
        }
        if (renderer_) {
            SDL_DestroyRenderer(renderer_);
            renderer_ = nullptr;
        }
        if (window_) {
            SDL_DestroyWindow(window_);
            window_ = nullptr;
        }
        SDL_Quit();
    }
    
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    uint32_t current_width_;
    uint32_t current_height_;
    uint64_t video_frames_;
    uint64_t audio_frames_;
    bool running_;
    std::chrono::steady_clock::time_point start_time_;
};

// SDL2 requires main to return int and take argc/argv on Windows
int main(int argc, char* argv[]) {
    // Initialize logger
    Logger::init("visual_viewer.log");
    Logger::log(Logger::LogLevel::INFO, "Visual Viewer started");
    
    // Parse command line arguments
    std::string server_address = "127.0.0.1";
    int server_port = 9999;
    
    if (argc > 1) {
        server_address = argv[1];
    }
    if (argc > 2) {
        server_port = std::atoi(argv[2]);
    }
    
    // Create viewer
    VisualViewer viewer;
    Logger::log(Logger::LogLevel::INFO, "About to initialize viewer...");
    if (!viewer.init()) {
        std::cerr << "Failed to initialize viewer" << std::endl;
        Logger::log(Logger::LogLevel::ERROR_LEVEL, "Failed to initialize viewer");
        Logger::shutdown();
        return 1;
    }
    
    Logger::log(Logger::LogLevel::INFO, "Viewer initialized successfully");
    
    // Create client
    StreamClient client(server_address, server_port);
    
    // Set callbacks
    Logger::log(Logger::LogLevel::INFO, "Setting up callbacks...");
    client.setVideoFrameCallback([&viewer](const VideoFrame& frame, const std::vector<uint8_t>&) {
        viewer.onVideoFrame(frame);
    });
    
    client.setAudioFrameCallback([&viewer](const AudioFrame& frame, const std::vector<uint8_t>&) {
        viewer.onAudioFrame(frame);
    });
    
    client.setDisconnectCallback([&viewer]() {
        viewer.onDisconnect();
    });
    
    // Connect to server
    std::cout << "Connecting to server at " << server_address << ":" << server_port << "...\n";
    Logger::log(Logger::LogLevel::INFO, "Attempting to connect to server...");
    if (!client.connect()) {
        std::cerr << "Failed to connect to server\n";
        Logger::log(Logger::LogLevel::ERROR_LEVEL, "Connection failed");
        Logger::shutdown();
        return 1;
    }
    
    std::cout << "✓ Connected successfully! Displaying stream...\n";
    Logger::log(Logger::LogLevel::INFO, "Successfully connected, waiting for frames...");
    std::cout << "Press ESC or close window to exit.\n\n";
    Logger::log(Logger::LogLevel::INFO, "Connected successfully, entering main loop");
    
    // Main loop
    while (viewer.isRunning() && client.isConnected()) {
        viewer.handleEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS UI
    }
    
    // Disconnect
    client.disconnect();
    
    // Print final stats
    viewer.printStats();
    
    Logger::log(Logger::LogLevel::INFO, "Visual Viewer stopped");
    Logger::shutdown();
    
    return 0;
}
