#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>

class Logger {
public:
    enum class LogLevel {
        DEBUG,
        INFO,
        WARN,
        ERROR_LEVEL
    };

    // Méthodes statiques pour usage global
    static void init(const std::string& filename, LogLevel min_level = LogLevel::INFO);
    static void shutdown();
    static void log(LogLevel level, const std::string& message);
    
    // Méthodes d'instance (ancien style)
    Logger(const std::string& filename);
    ~Logger();
    void log(const std::string& message, LogLevel level = LogLevel::INFO);

private:
    static std::mutex mutex_;
    static std::ofstream file_;
    static LogLevel min_level_;
    
    std::ofstream logFile;
    std::mutex logMutex;
    std::string logFileName;

    static const char* level_to_string(LogLevel level);
    std::string logLevelToString(LogLevel level);
};

// Macros pour faciliter l'usage
#define LOG_DEBUG(msg) Logger::log(Logger::LogLevel::DEBUG, msg)
#define LOG_INFO(msg) Logger::log(Logger::LogLevel::INFO, msg)
#define LOG_WARN(msg) Logger::log(Logger::LogLevel::WARN, msg)
#define LOG_ERROR(msg) Logger::log(Logger::LogLevel::ERROR_LEVEL, msg)

#endif // LOGGER_H