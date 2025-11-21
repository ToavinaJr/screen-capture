#include <iostream>

class TestLogger {
public:
    enum class LogLevel { INFO, ERROR };
    
    static void log(LogLevel level, const char* msg) {
        std::cout << msg << std::endl;
    }
};

int main() {
    TestLogger::log(TestLogger::LogLevel::ERROR, "Test");
    return 0;
}
