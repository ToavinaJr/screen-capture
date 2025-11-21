#include "../src/capture/ScreenCapture.h"
#include "../src/utils/Logger.h"
#include <iostream>

int main() {
    Logger::init("test_screen_capture.log");
    Logger::log(Logger::LogLevel::INFO, "Testing screen capture");
    
    ScreenCapture capture;
    if (!capture.init()) {
        std::cerr << "Failed to initialize screen capture: " << capture.getLastError() << std::endl;
        return 1;
    }
    
    std::cout << "Screen capture initialized successfully" << std::endl;
    
    int width, height;
    if (!capture.getScreenDimensions(width, height)) {
        std::cerr << "Failed to get screen dimensions" << std::endl;
        return 1;
    }
    
    std::cout << "Screen dimensions: " << width << "x" << height << std::endl;
    
    // Try to capture a small region first
    std::cout << "Capturing 100x100 region..." << std::endl;
    std::vector<uint8_t> pixels = capture.captureRegion(0, 0, 100, 100);
    
    if (pixels.empty()) {
        std::cerr << "Failed to capture screen: " << capture.getLastError() << std::endl;
        return 1;
    }
    
    std::cout << "Successfully captured " << pixels.size() << " bytes" << std::endl;
    std::cout << "First few bytes: ";
    for (int i = 0; i < 16 && i < pixels.size(); i++) {
        std::cout << std::hex << (int)pixels[i] << " ";
    }
    std::cout << std::dec << std::endl;
    
    // Try full screen
    std::cout << "Capturing full screen..." << std::endl;
    pixels = capture.captureScreen(width, height);
    
    if (pixels.empty()) {
        std::cerr << "Failed to capture full screen: " << capture.getLastError() << std::endl;
        return 1;
    }
    
    std::cout << "Successfully captured full screen: " << width << "x" << height 
              << " (" << pixels.size() << " bytes)" << std::endl;
    
    Logger::shutdown();
    return 0;
}
