#pragma once

#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <memory>

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
     * @brief Initialize the logger from environment variables.
     *
     * `LOG_LEVEL` overrides the default severity when valid.
     * `LOG_OUTPUT` controls console/file/both output and defaults to file.
     * `LOG_FILE` sets the file path when file logging is enabled.
     */
    void init();

    /**
     * @brief Initialize the logger with an explicit default level.
     *
     * `LOG_LEVEL` still overrides the provided level when valid.
     */
    void init(LogLevel level);

    /**
     * @brief Shut down the logger and close any active file handles.
     *
     * Further log calls are ignored until the logger is initialized again.
     */
    void shutdown();

    /**
     * @brief Enable or disable file logging
     * @param filename If not empty, enables file logging to the specified file
     */
    bool enableFileLogging(const std::string& filename = "");

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
     * @brief Log a debug message with file and line information
     * @param message The message to log
     * @param file Source file name (use __FILE__)
     * @param line Source line number (use __LINE__)
     * @param module Optional module name
     */
    void debug(const std::string& message, const std::string& file = "", int line = 0, const std::string& module = "");

    /**
     * @brief Log an info message with file and line information
     * @param message The message to log
     * @param file Source file name (use __FILE__)
     * @param line Source line number (use __LINE__)
     * @param module Optional module name
     */
    void info(const std::string& message, const std::string& file = "", int line = 0, const std::string& module = "");

    /**
     * @brief Log a warning message with file and line information
     * @param message The message to log
     * @param file Source file name (use __FILE__)
     * @param line Source line number (use __LINE__)
     * @param module Optional module name
     */
    void warn(const std::string& message, const std::string& file = "", int line = 0, const std::string& module = "");

    /**
     * @brief Log an error message with file and line information
     * @param message The message to log
     * @param file Source file name (use __FILE__)
     * @param line Source line number (use __LINE__)
     * @param module Optional module name
     */
    void error(const std::string& message, const std::string& file = "", int line = 0, const std::string& module = "");

    /**
     * @brief Log a message with a specific level and context information
     * @param level The log level
     * @param message The message to log
     * @param file Source file name (use __FILE__)
     * @param line Source line number (use __LINE__)
     * @param module Optional module name
     */
    void log(LogLevel level, const std::string& message, const std::string& file = "", int line = 0, const std::string& module = "");

    // Delete copy constructor and assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger();
    ~Logger();

    void initWithDefaults(std::optional<LogLevel> defaultLevel);
    void resetOutputsLocked();
    bool openLogFileLocked(const std::string& filename);
    std::string makeDefaultLogFilename() const;

    /**
     * @brief Internal logging function with context information
     * @param level The log level
     * @param message The message to log
     * @param file Source file name
     * @param line Source line number
     * @param module Optional module name
     */
    void logInternal(LogLevel level, const std::string& message, const std::string& file, int line, const std::string& module);

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
     * @brief Extract filename from full path
     * @param path Full file path
     * @return Just the filename without directory
     */
    std::string extractFilename(const std::string& path) const;

    // Member variables
    LogLevel minLevel_;
    std::unique_ptr<std::ofstream> fileStream_;
    std::mutex logMutex_;
    bool initialized_;
    bool shutdown_;
    bool consoleEnabled_;
    bool fileEnabled_;
};

/**
 * @brief Convenience macros for easy logging with automatic file and line capture
 */
#define LOG_DEBUG(message) myproject::Logger::getInstance().debug(message, __FILE__, __LINE__)
#define LOG_INFO(message) myproject::Logger::getInstance().info(message, __FILE__, __LINE__)
#define LOG_WARN(message) myproject::Logger::getInstance().warn(message, __FILE__, __LINE__)
#define LOG_ERROR(message) myproject::Logger::getInstance().error(message, __FILE__, __LINE__)

/**
 * @brief Convenience macros for easy logging with module name
 */
#define LOG_DEBUG_MODULE(message, module) myproject::Logger::getInstance().debug(message, __FILE__, __LINE__, module)
#define LOG_INFO_MODULE(message, module) myproject::Logger::getInstance().info(message, __FILE__, __LINE__, module)
#define LOG_WARN_MODULE(message, module) myproject::Logger::getInstance().warn(message, __FILE__, __LINE__, module)
#define LOG_ERROR_MODULE(message, module) myproject::Logger::getInstance().error(message, __FILE__, __LINE__, module)

} // namespace myproject
