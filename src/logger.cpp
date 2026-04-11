#include "myproject/logger.h"

namespace myproject {

// Static member initialization
Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() 
    : minLevel_(LogLevel::INFO)
    , useColors_(false)
    , initialized_(false) {
}

Logger::~Logger() {
    if (fileStream_ && fileStream_->is_open()) {
        fileStream_->close();
    }
}

void Logger::init(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex_);
    minLevel_ = level;
    initialized_ = true;
    
    // 直接输出，不调用logInternal避免死锁
    std::string message = "Logger initialized with level: " + levelToString(level);
    std::string timestamp = getTimestamp();
    std::string formattedMessage = "[" + timestamp + "] [INFO] " + message;
    std::cout << formattedMessage << std::endl;
}

void Logger::enableFileLogging(const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex_);
    
    if (filename.empty()) {
        // 默认文件名带时间戳
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now = *std::localtime(&time_t_now);
        
        std::ostringstream oss;
        oss << "log_" 
            << (tm_now.tm_year + 1900) << "-"
            << std::setfill('0') << std::setw(2) << (tm_now.tm_mon + 1) << "-"
            << std::setfill('0') << std::setw(2) << tm_now.tm_mday << "_"
            << std::setfill('0') << std::setw(2) << tm_now.tm_hour << "-"
            << std::setfill('0') << std::setw(2) << tm_now.tm_min << "-"
            << std::setfill('0') << std::setw(2) << tm_now.tm_sec
            << ".txt";
        
        fileStream_ = std::make_unique<std::ofstream>(oss.str());
    } else {
        fileStream_ = std::make_unique<std::ofstream>(filename);
    }
    
    if (fileStream_->is_open()) {
        // 直接输出，不调用logInternal
        std::string message = "File logging enabled to: " + 
                             (filename.empty() ? "auto-generated file" : filename);
        std::string timestamp = getTimestamp();
        std::string formattedMessage = "[" + timestamp + "] [INFO] " + message;
        std::cout << formattedMessage << std::endl;
        
        // 如果需要也写入文件
        if (fileStream_) {
            *fileStream_ << formattedMessage << std::endl;
        }
    } else {
        // 直接输出错误
        std::string message = "Failed to open log file: " + filename;
        std::string timestamp = getTimestamp();
        std::string formattedMessage = "[" + timestamp + "] [ERROR] " + message;
        std::cout << formattedMessage << std::endl;
        fileStream_.reset();
    }
}

void Logger::disableFileLogging() {
    std::lock_guard<std::mutex> lock(logMutex_);
    if (fileStream_ && fileStream_->is_open()) {
        fileStream_->close();
        fileStream_.reset();
        
        // 直接输出，不调用logInternal
        std::string message = "File logging disabled";
        std::string timestamp = getTimestamp();
        std::string formattedMessage = "[" + timestamp + "] [INFO] " + message;
        std::cout << formattedMessage << std::endl;
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex_);
    minLevel_ = level;
    
    // 直接输出，不调用logInternal
    std::string message = "Log level changed to: " + levelToString(level);
    std::string timestamp = getTimestamp();
    std::string formattedMessage = "[" + timestamp + "] [INFO] " + message;
    std::cout << formattedMessage << std::endl;
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (!initialized_) {
        init();
    }
    
    if (level >= minLevel_) {
        logInternal(level, message);
    }
}

void Logger::logInternal(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex_);
    
    std::string timestamp = getTimestamp();
    std::string levelStr = levelToString(level);
    std::string formattedMessage = "[" + timestamp + "] [" + levelStr + "] " + message;
    
    // 输出到控制台
    if (useColors_) {
        std::cout << getLevelColor(level) << formattedMessage << getResetColor() << std::endl;
    } else {
        std::cout << formattedMessage << std::endl;
    }
    
    // 输出到文件（无颜色）
    if (fileStream_ && fileStream_->is_open()) {
        *fileStream_ << formattedMessage << std::endl;
        fileStream_->flush();
    }
}

std::string Logger::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ) % 1000;
    
    std::tm tm_now = *std::localtime(&time_t_now);
    
    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string Logger::getLevelColor(LogLevel level) const {
    if (!useColors_) return "";
    
    switch (level) {
        case LogLevel::DEBUG: return "\033[36m";  // 青色
        case LogLevel::INFO:  return "\033[32m";  // 绿色
        case LogLevel::WARN:  return "\033[33m";  // 黄色
        case LogLevel::ERROR: return "\033[31m";  // 红色
        default: return "\033[0m";  // 重置
    }
}

std::string Logger::getResetColor() const {
    return useColors_ ? "\033[0m" : "";
}

} // namespace myproject