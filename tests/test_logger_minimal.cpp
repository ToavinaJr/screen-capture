#include <iostream>
#include "../src/utils/Logger.h"

int main() {
    std::cout << "=== Test Logger Minimal ===\n";
    
    try {
        Logger::init("test.log");
        
        LOG_INFO("Test INFO message");
        LOG_WARN("Test WARN message");
        LOG_ERROR("Test ERROR message");
        
        std::cout << "✓ Logger fonctionne!\n";
        
        Logger::shutdown();
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "✗ Erreur: " << e.what() << std::endl;
        return 1;
    }
}
