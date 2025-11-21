#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <cmath>
#include "../src/network/StreamServer.h"
#include "../src/utils/Logger.h"
#include "common.h"

int main() {
    std::cout << "=== Stream Server with Simulated Screen Capture ===\n\n";
    
    Logger::init("stream_server.log");
    
    const int PORT = 9999;
    const int FPS = 30;
    const int WIDTH = 1280;
    const int HEIGHT = 720;
    
    // Create server
    std::cout << "[1] Starting StreamServer on port " << PORT << "...\n";
    StreamServer server("0.0.0.0", PORT);
    
    if (!server.start()) {
        std::cerr << "Failed to start server!\n";
        Logger::shutdown();
        return 1;
    }
    
    std::cout << "✓ Server started successfully\n";
    std::cout << "Clients can connect to localhost:" << PORT << "\n\n";
    
    std::cout << "[2] Starting simulated screen capture at " << FPS << " FPS...\n";
    std::cout << "Press Ctrl+C to stop\n\n";
    
    uint32_t frame_counter = 0;
    uint32_t audio_counter = 0;
    const int frame_time_ms = 1000 / FPS;
    const int audio_frame_time_ms = 100; // 10 audio frames per second
    auto last_audio_time = std::chrono::steady_clock::now();
    
    while (true) {
        auto start = std::chrono::steady_clock::now();
        
        // Create simulated frame
        VideoFrame frame;
        frame.frame_number = frame_counter++;
        frame.width = WIDTH;
        frame.height = HEIGHT;
        frame.quality = 80;
        frame.timestamp = get_timestamp_us();
        
        // Simulated compressed frame data (much smaller than raw pixels)
        size_t compressed_size = (WIDTH * HEIGHT * 3) / 20; // ~5% of raw size
        frame.data.resize(compressed_size);
        
        // Fill with some pattern
        for (size_t i = 0; i < compressed_size; i++) {
            frame.data[i] = (uint8_t)((i + frame_counter) % 256);
        }
        
        // Broadcast frame
        server.broadcastVideoFrame(frame);
        
        // Broadcast audio frame periodically
        auto now = std::chrono::steady_clock::now();
        auto audio_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_audio_time).count();
        
        if (audio_elapsed >= audio_frame_time_ms) {
            last_audio_time = now;
            
            AudioFrame audio_frame;
            audio_frame.frame_number = audio_counter++;
            audio_frame.sample_rate = 44100;
            audio_frame.channels = 1;
            audio_frame.timestamp = get_timestamp_us();
            
            // Simulated audio samples (4096 samples = ~93ms at 44.1kHz)
            audio_frame.samples.resize(4096);
            for (int i = 0; i < 4096; i++) {
                audio_frame.samples[i] = sin(2.0 * 3.14159 * 440.0 * i / 44100.0) * 0.5f; // 440Hz tone
            }
            
            server.broadcastAudioFrame(audio_frame);
        }
        
        // Log every 60 frames (2 seconds)
        if (frame_counter % 60 == 0) {
            size_t clients = server.getClientCount();
            double data_mb = (frame_counter * compressed_size) / (1024.0 * 1024.0);
            
            std::cout << "Video: " << frame_counter 
                      << " | Audio: " << audio_counter
                      << " | Clients: " << clients
                      << " | Data: " << std::fixed << std::setprecision(2) << data_mb << " MB\n";
        }
        
        // Sleep to maintain FPS
        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        int sleep_time = frame_time_ms - (int)elapsed;
        
        if (sleep_time > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        }
        
        // Optional: run for limited time in testing
        if (frame_counter >= FPS * 60) { // 1 minute
            std::cout << "\nReached 1 minute limit, stopping...\n";
            break;
        }
    }
    
    std::cout << "\n[3] Shutting down server...\n";
    server.stop();
    
    std::cout << "\n=== Summary ===\n";
    std::cout << "Total video frames: " << frame_counter << "\n";
    std::cout << "Total audio frames: " << audio_counter << "\n";
    std::cout << "Duration: " << (frame_counter / FPS) << " seconds\n";
    
    Logger::shutdown();
    return 0;
}
