#include "Logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>

std::mutex Logger::mutex_;
std::ofstream Logger::file_;
Logger::LogLevel Logger::min_level_ = Logger::LogLevel::INFO;

void Logger::init(const std::string& filename, LogLevel min_level) {
    min_level_ = min_level;
    
    if (file_.is_open()) {
        file_.close();
    }
    
    file_.open(filename, std::ios::out | std::ios::app);
    
    if (file_.is_open()) {
        file_ << "Logger initialized" << std::endl;
    }
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (file_.is_open()) {
        file_ << "Logger shutdown" << std::endl;
        file_.close();
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < min_level_) return;
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    ss << " [" << level_to_string(level) << "] " << message;
    
    std::string log_line = ss.str();
    
    if (level >= LogLevel::WARN) {
        std::cerr << log_line << std::endl;
    } else {
        std::cout << log_line << std::endl;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_ << log_line << std::endl;
        file_.flush();
    }
}

const char* Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR_LEVEL: return "ERROR";
        default: return "UNKNOWN";
    }
}

Logger::Logger(const std::string& filename) : logFileName(filename) {
    logFile.open(filename, std::ios::app);
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Logger::log(const std::string& message, LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (logFile.is_open()) {
        logFile << message << std::endl;
    }
    
    std::cout << message << std::endl;
}

std::string Logger::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR_LEVEL: return "ERROR";
        default: return "UNKNOWN";
    }
}
