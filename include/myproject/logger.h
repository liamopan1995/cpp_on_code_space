#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace myproject {

/**
 * @brief Log levels for the logger
 */
enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

/**
 * @brief A simple, thread-safe logger class
 * 
 * Supports multiple log levels and can output to console and/or file.
 * Singleton pattern ensures only one logger instance exists.
 */
class Logger {
public:
    /**
     * @brief Get the singleton instance of the logger
     */
    static Logger& getInstance();

    /**
     * @brief Initialize the logger with a minimum log level
     * @param level Minimum log level to output (messages below this level are ignored)
     */
    void init(LogLevel level = LogLevel::INFO);

    /**
     * @brief Enable or disable file logging
     * @param filename If not empty, enables file logging to the specified file
     */
    void enableFileLogging(const std::string& filename = "");

    /**
     * @brief Disable file logging
     */
    void disableFileLogging();

    /**
     * @brief Set the minimum log level
     * @param level Minimum log level to output
     */
    void setLogLevel(LogLevel level);

    /**
     * @brief Log a debug message
     * @param message The message to log
     */
    void debug(const std::string& message);

    /**
     * @brief Log an info message
     * @param message The message to log
     */
    void info(const std::string& message);

    /**
     * @brief Log a warning message
     * @param message The message to log
     */
    void warn(const std::string& message);

    /**
     * @brief Log an error message
     * @param message The message to log
     */
    void error(const std::string& message);

    /**
     * @brief Log a message with a specific level
     * @param level The log level
     * @param message The message to log
     */
    void log(LogLevel level, const std::string& message);

    // Delete copy constructor and assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger();
    ~Logger();

    /**
     * @brief Internal logging function
     * @param level The log level
     * @param message The message to log
     */
    void logInternal(LogLevel level, const std::string& message);

    /**
     * @brief Get current timestamp as string
     * @return Formatted timestamp string
     */
    std::string getTimestamp() const;

    /**
     * @brief Convert log level to string
     * @param level The log level
     * @return String representation of the log level
     */
    std::string levelToString(LogLevel level) const;

    /**
     * @brief Get color code for log level (for console output)
     * @param level The log level
     * @return ANSI color code string
     */
    std::string getLevelColor(LogLevel level) const;

    /**
     * @brief Reset color code (for console output)
     * @return ANSI reset code string
     */
    std::string getResetColor() const;

    // Member variables
    LogLevel minLevel_;
    std::unique_ptr<std::ofstream> fileStream_;
    std::mutex logMutex_;
    bool useColors_;
    bool initialized_;
};

/**
 * @brief Convenience macros for easy logging
 */
#define LOG_DEBUG(message) myproject::Logger::getInstance().debug(message)
#define LOG_INFO(message) myproject::Logger::getInstance().info(message)
#define LOG_WARN(message) myproject::Logger::getInstance().warn(message)
#define LOG_ERROR(message) myproject::Logger::getInstance().error(message)

} // namespace myproject