#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include "../src/network/StreamServer.h"
#include "../src/network/StreamClient.h"
#include "../src/utils/Logger.h"

void printStats(const std::string& prefix, uint64_t video_frames, uint64_t audio_frames, uint64_t bytes) {
    std::cout << prefix 
              << " Video: " << video_frames 
              << " | Audio: " << audio_frames
              << " | Bytes: " << std::fixed << std::setprecision(2) << (bytes / 1024.0) << " KB\n";
}

int main() {
    std::cout << "=== End-to-End Streaming Test ===\n\n";
    
    Logger::init("e2e_test.log");
    
    // Test configuration
    const std::string SERVER_ADDRESS = "127.0.0.1";
    const int SERVER_PORT = 9999;
    const int TEST_DURATION = 15; // seconds
    const int BROADCAST_INTERVAL = 100; // ms
    
    // Create server
    std::cout << "[1] Creating and starting server...\n";
    StreamServer server(SERVER_ADDRESS, SERVER_PORT);
    
    if (!server.start()) {
        std::cerr << "Failed to start server!\n";
        Logger::shutdown();
        return 1;
    }
    
    std::cout << "✓ Server started on " << SERVER_ADDRESS << ":" << SERVER_PORT << "\n\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Create client
    std::cout << "[2] Creating and connecting client...\n";
    StreamClient client(SERVER_ADDRESS, SERVER_PORT);
    
    // Setup callbacks
    uint64_t client_video_received = 0;
    uint64_t client_audio_received = 0;
    
    client.setVideoFrameCallback([&](const VideoFrame& frame, const std::vector<uint8_t>& data) {
        client_video_received++;
        if (client_video_received % 10 == 0) {
            std::cout << "  Client received video frame " << client_video_received 
                      << " (" << frame.width << "x" << frame.height << ", " 
                      << data.size() << " bytes)\n";
        }
    });
    
    client.setAudioFrameCallback([&](const AudioFrame& frame, const std::vector<uint8_t>& data) {
        client_audio_received++;
        if (client_audio_received % 20 == 0) {
            std::cout << "  Client received audio frame " << client_audio_received 
                      << " (" << frame.sample_rate << "Hz, " 
                      << data.size() << " bytes)\n";
        }
    });
    
    client.setDisconnectCallback([]() {
        std::cout << "  Client disconnected by server\n";
    });
    
    if (!client.connect()) {
        std::cerr << "Failed to connect client!\n";
        server.stop();
        Logger::shutdown();
        return 1;
    }
    
    std::cout << "✓ Client connected successfully\n\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "[3] Starting broadcast simulation for " << TEST_DURATION << " seconds...\n";
    std::cout << "Server will broadcast video and audio frames\n\n";
    
    // Broadcast loop
    auto start_time = std::chrono::steady_clock::now();
    uint64_t video_frame_count = 0;
    uint64_t audio_frame_count = 0;
    
    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time).count();
        
        if (elapsed >= TEST_DURATION) {
            break;
        }
        
        // Broadcast video frame (30 FPS = ~33ms)
        if (video_frame_count % 3 == 0) {
            VideoFrame video_frame;
            video_frame.frame_number = video_frame_count;
            video_frame.width = 1280;
            video_frame.height = 720;
            video_frame.quality = 80;
            video_frame.timestamp = get_timestamp_us();
            video_frame.data.resize(1280 * 720 * 3 / 10); // Simulated compressed data
            
            server.broadcastVideoFrame(video_frame);
            video_frame_count++;
        }
        
        // Broadcast audio frame (more frequent than video)
        AudioFrame audio_frame;
        audio_frame.frame_number = audio_frame_count;
        audio_frame.sample_rate = 44100;
        audio_frame.channels = 1;
        audio_frame.timestamp = get_timestamp_us();
        audio_frame.samples.resize(4096); // 4096 samples
        
        server.broadcastAudioFrame(audio_frame);
        audio_frame_count++;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(BROADCAST_INTERVAL));
        
        // Print progress every 3 seconds
        if (elapsed > 0 && elapsed % 3 == 0 && 
            elapsed != std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start_time - std::chrono::seconds(1)).count()) {
            std::cout << "\n[Progress] Time: " << elapsed << "s / " << TEST_DURATION << "s\n";
            std::cout << "  Server broadcast: " << video_frame_count << " video, " 
                      << audio_frame_count << " audio\n";
            std::cout << "  Client received: " << client_video_received << " video, " 
                      << client_audio_received << " audio\n";
            std::cout << "  Active clients: " << server.getClientCount() << "\n\n";
        }
    }
    
    std::cout << "\n[4] Test completed, collecting final statistics...\n\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Final statistics
    std::cout << "=== Final Statistics ===\n\n";
    
    std::cout << "Server:\n";
    std::cout << "  Broadcast video frames: " << video_frame_count << "\n";
    std::cout << "  Broadcast audio frames: " << audio_frame_count << "\n";
    std::cout << "  Connected clients: " << server.getClientCount() << "\n\n";
    
    std::cout << "Client:\n";
    std::cout << "  Received video frames: " << client.getReceivedVideoFrames() << "\n";
    std::cout << "  Received audio frames: " << client.getReceivedAudioFrames() << "\n";
    std::cout << "  Total bytes received: " << std::fixed << std::setprecision(2) 
              << (client.getBytesReceived() / 1024.0) << " KB\n\n";
    
    // Calculate success rate
    double video_success = (client.getReceivedVideoFrames() * 100.0) / video_frame_count;
    double audio_success = (client.getReceivedAudioFrames() * 100.0) / audio_frame_count;
    
    std::cout << "Success Rate:\n";
    std::cout << "  Video: " << std::fixed << std::setprecision(1) << video_success << "%\n";
    std::cout << "  Audio: " << std::fixed << std::setprecision(1) << audio_success << "%\n\n";
    
    // Cleanup
    std::cout << "[5] Shutting down...\n";
    client.disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    server.stop();
    
    std::cout << "\n=== Test Result ===\n";
    if (video_success > 95.0 && audio_success > 95.0) {
        std::cout << "✓ SUCCESS: End-to-end streaming working perfectly!\n";
    } else {
        std::cout << "⚠ WARNING: Some frame loss detected\n";
    }
    
    Logger::shutdown();
    return 0;
}
