#include <iostream>
#include <SDL2/SDL.h>
#include "../src/audio/MicrophoneCapture.h"
#include "../src/utils/Logger.h"
#include <thread>
#include <chrono>
#include <atomic>

std::atomic<size_t> totalBytesReceived(0);
std::atomic<int> callbackCount(0);

void audioDataCallback(const void* data, size_t size) {
    totalBytesReceived += size;
    callbackCount++;
    
    // Log every 100 callbacks to avoid spam
    if (callbackCount % 100 == 0) {
        std::cout << "Received " << callbackCount.load() << " audio chunks, "
                  << "Total: " << totalBytesReceived.load() << " bytes" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    Logger::init("test_microphone.log");
    
    std::cout << "=== Microphone Capture Test ===" << std::endl;
    std::cout << std::endl;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    // List available audio devices
    int numDevices = SDL_GetNumAudioDevices(1);  // 1 = recording devices
    std::cout << "Available audio input devices: " << numDevices << std::endl;
    for (int i = 0; i < numDevices; i++) {
        const char* deviceName = SDL_GetAudioDeviceName(i, 1);
        std::cout << "  [" << i << "] " << (deviceName ? deviceName : "Unknown") << std::endl;
    }
    std::cout << std::endl;

    // Create microphone capture
    MicrophoneCapture mic;

    // Start capture
    std::cout << "Starting microphone capture for 5 seconds..." << std::endl;
    if (!mic.startCapture(audioDataCallback)) {
        std::cerr << "Failed to start microphone capture!" << std::endl;
        SDL_Quit();
        Logger::shutdown();
        return 1;
    }

    std::cout << "Recording... (speak into your microphone)" << std::endl;

    // Record for 5 seconds
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Stop capture
    mic.stopCapture();

    // Print statistics
    std::cout << std::endl;
    std::cout << "=== Recording Statistics ===" << std::endl;
    std::cout << "Total callbacks: " << callbackCount.load() << std::endl;
    std::cout << "Total bytes: " << totalBytesReceived.load() << std::endl;
    std::cout << "Average bytes per callback: " 
              << (callbackCount > 0 ? totalBytesReceived.load() / callbackCount.load() : 0) 
              << std::endl;

    if (callbackCount > 0) {
        std::cout << std::endl;
        std::cout << "SUCCESS: Microphone capture working!" << std::endl;
    } else {
        std::cout << std::endl;
        std::cout << "WARNING: No audio data received. Check microphone permissions." << std::endl;
    }

    SDL_Quit();
    Logger::shutdown();
    return 0;
}
