#include "../include/common.h"
#include <iostream>

int main() {
    std::cout << "=== Test Common Headers ===\n\n";
    
    #ifdef PLATFORM_WINDOWS
    std::cout << "✓ Platform: Windows\n";
    #endif
    
    #ifdef PLATFORM_LINUX
    std::cout << "✓ Platform: Linux\n";
    #endif
    
    std::cout << "✓ Magic Number: 0x" << std::hex << MAGIC_NUMBER << std::dec << "\n";
    std::cout << "✓ Protocol Version: " << (int)PROTOCOL_VERSION << "\n";
    std::cout << "✓ Default Port: " << Config::DEFAULT_PORT << "\n";
    std::cout << "✓ Default FPS: " << Config::DEFAULT_FPS << "\n";
    
    // Test timestamp
    uint64_t ts = get_timestamp_us();
    std::cout << "✓ Timestamp: " << ts << " µs\n";
    
    // Test SocketInitializer
    try {
        SocketInitializer init;
        std::cout << "✓ Socket initialized\n";
    } catch (const std::exception& e) {
        std::cerr << "✗ Socket init failed: " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "\n✓ Tous les tests réussis!\n";
    return 0;
}
