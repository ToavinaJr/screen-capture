#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include "../src/network/StreamClient.h"
#include "../src/utils/Logger.h"

void printProgress(uint64_t video_frames, uint64_t audio_frames, uint64_t bytes, 
                  int width, int height, int fps, double duration) {
    std::cout << "\r";
    std::cout << "Time: " << std::fixed << std::setprecision(1) << duration << "s | ";
    std::cout << "Video: " << video_frames << " frames | ";
    std::cout << "Audio: " << audio_frames << " frames | ";
    std::cout << "Size: " << width << "x" << height << " | ";
    std::cout << "FPS: " << fps << " | ";
    std::cout << "Data: " << std::fixed << std::setprecision(2) << (bytes / 1024.0 / 1024.0) << " MB";
    std::cout << std::flush;
}

int main(int argc, char* argv[]) {
    std::cout << "=== Screen Share Viewer ===\n\n";
    
    Logger::init("viewer.log");
    
    // Allow server address to be specified as command line argument
    std::string SERVER_ADDRESS = "127.0.0.1";
    if (argc > 1) {
        SERVER_ADDRESS = argv[1];
    }
    const int SERVER_PORT = 9999;
    
    std::cout << "Connecting to " << SERVER_ADDRESS << ":" << SERVER_PORT << "...\n";
    
    StreamClient client(SERVER_ADDRESS, SERVER_PORT);
    
    // Statistics
    uint64_t video_count = 0;
    uint64_t audio_count = 0;
    int last_width = 0;
    int last_height = 0;
    auto start_time = std::chrono::steady_clock::now();
    auto last_frame_time = start_time;
    int current_fps = 0;
    
    // Setup callbacks
    client.setVideoFrameCallback([&](const VideoFrame& frame, const std::vector<uint8_t>& data) {
        video_count++;
        last_width = frame.width;
        last_height = frame.height;
        
        // Calculate FPS
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame_time).count();
        if (elapsed > 0) {
            current_fps = (int)(1000.0 / elapsed);
        }
        last_frame_time = now;
        
        // Print progress every frame
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        printProgress(video_count, audio_count, client.getBytesReceived(), 
                     last_width, last_height, current_fps, duration);
    });
    
    client.setAudioFrameCallback([&](const AudioFrame& frame, const std::vector<uint8_t>& data) {
        audio_count++;
    });
    
    client.setDisconnectCallback([]() {
        std::cout << "\n\nDisconnected from server.\n";
    });
    
    if (!client.connect()) {
        std::cerr << "Failed to connect to server!\n";
        std::cerr << "Make sure the screen_share application is running.\n";
        Logger::shutdown();
        return 1;
    }
    
    std::cout << "✓ Connected successfully!\n";
    std::cout << "Receiving stream... (Press Ctrl+C to stop)\n\n";
    
    // Run for a specified time or until disconnected
    while (client.isConnected()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Optional: add timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time).count();
        
        if (elapsed > 300) { // 5 minutes max
            std::cout << "\n\nTimeout reached (5 minutes).\n";
            break;
        }
    }
    
    std::cout << "\n\n=== Final Statistics ===\n";
    std::cout << "Total video frames: " << client.getReceivedVideoFrames() << "\n";
    std::cout << "Total audio frames: " << client.getReceivedAudioFrames() << "\n";
    std::cout << "Total data received: " << std::fixed << std::setprecision(2) 
              << (client.getBytesReceived() / 1024.0 / 1024.0) << " MB\n";
    std::cout << "Resolution: " << last_width << "x" << last_height << "\n";
    
    auto total_duration = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start_time).count();
    if (total_duration > 0) {
        double avg_fps = (double)client.getReceivedVideoFrames() / total_duration;
        std::cout << "Average FPS: " << std::fixed << std::setprecision(1) << avg_fps << "\n";
    }
    
    client.disconnect();
    Logger::shutdown();
    
    return 0;
}
