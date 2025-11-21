#include <iostream>
#include <exception>
#include "core/Application.h"
#include "utils/Logger.h"

int main(int argc, char* argv[]) {
    try {
        Application app;
        app.init();
        app.run();
        app.shutdown();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        LOG_ERROR(std::string("Fatal error: ") + e.what());
        return -1;
    }
}