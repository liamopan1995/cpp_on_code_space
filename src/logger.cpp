#include "myproject/logger.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>

namespace myproject {
namespace {

std::string normalizeToken(const char* value) {
    if (value == nullptr) {
        return "";
    }

    std::string token(value);
    std::transform(token.begin(), token.end(), token.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return token;
}

std::optional<LogLevel> parseLogLevelToken(const char* value) {
    const std::string token = normalizeToken(value);
    if (token == "debug") {
        return LogLevel::DEBUG;
    }
    if (token == "info") {
        return LogLevel::INFO;
    }
    if (token == "warn" || token == "warning") {
        return LogLevel::WARN;
    }
    if (token == "error") {
        return LogLevel::ERROR;
    }
    return std::nullopt;
}

}  // namespace

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger()
    : minLevel_(LogLevel::WARN),
      initialized_(false),
      shutdown_(false),
      consoleEnabled_(false),
      fileEnabled_(false) {
}

Logger::~Logger() {
    shutdown();
}

void Logger::init() {
    initWithDefaults(std::nullopt);
}

void Logger::init(LogLevel level) {
    initWithDefaults(level);
}

void Logger::initWithDefaults(std::optional<LogLevel> defaultLevel) {
    std::lock_guard<std::mutex> lock(logMutex_);

    resetOutputsLocked();
    shutdown_ = false;
    initialized_ = true;

    minLevel_ = defaultLevel.value_or(LogLevel::WARN);

    const char* levelEnv = std::getenv("LOG_LEVEL");
    const std::optional<LogLevel> envLevel = parseLogLevelToken(levelEnv);
    if (envLevel.has_value()) {
        minLevel_ = *envLevel;
    } else if (levelEnv != nullptr && levelEnv[0] != '\0') {
        minLevel_ = LogLevel::WARN;
    }

    const std::string outputMode = normalizeToken(std::getenv("LOG_OUTPUT"));
    if (outputMode == "console") {
        consoleEnabled_ = true;
        fileEnabled_ = false;
    } else if (outputMode == "both") {
        consoleEnabled_ = true;
        fileEnabled_ = true;
    } else {
        consoleEnabled_ = false;
        fileEnabled_ = true;
    }

    if (!fileEnabled_) {
        return;
    }

    const char* fileEnv = std::getenv("LOG_FILE");
    const std::string path = (fileEnv != nullptr && fileEnv[0] != '\0')
        ? std::string(fileEnv)
        : makeDefaultLogFilename();

    if (!openLogFileLocked(path)) {
        fileEnabled_ = false;
        consoleEnabled_ = true;
    }
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(logMutex_);
    resetOutputsLocked();
    initialized_ = false;
    shutdown_ = true;
}

bool Logger::enableFileLogging(const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex_);

    if (shutdown_) {
        shutdown_ = false;
        initialized_ = true;
    } else if (!initialized_) {
        initialized_ = true;
    }

    fileEnabled_ = openLogFileLocked(filename.empty() ? makeDefaultLogFilename() : filename);
    return fileEnabled_;
}

void Logger::disableFileLogging() {
    std::lock_guard<std::mutex> lock(logMutex_);
    fileEnabled_ = false;
    if (fileStream_ && fileStream_->is_open()) {
        fileStream_->close();
    }
    fileStream_.reset();
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex_);
    minLevel_ = level;
}

void Logger::debug(const std::string& message, const std::string& file, int line, const std::string& module) {
    log(LogLevel::DEBUG, message, file, line, module);
}

void Logger::info(const std::string& message, const std::string& file, int line, const std::string& module) {
    log(LogLevel::INFO, message, file, line, module);
}

void Logger::warn(const std::string& message, const std::string& file, int line, const std::string& module) {
    log(LogLevel::WARN, message, file, line, module);
}

void Logger::error(const std::string& message, const std::string& file, int line, const std::string& module) {
    log(LogLevel::ERROR, message, file, line, module);
}

void Logger::log(LogLevel level, const std::string& message, const std::string& file, int line, const std::string& module) {
    bool shouldInit = false;
    {
        std::lock_guard<std::mutex> lock(logMutex_);
        if (shutdown_) {
            return;
        }
        shouldInit = !initialized_;
    }

    if (shouldInit) {
        init();
    }

    std::lock_guard<std::mutex> lock(logMutex_);
    if (shutdown_ || !initialized_ || level < minLevel_) {
        return;
    }

    logInternal(level, message, file, line, module);
}

void Logger::logInternal(LogLevel level, const std::string& message, const std::string& file, int line, const std::string& module) {
    std::ostringstream formatted;
    formatted << "[" << getTimestamp() << "] [" << levelToString(level) << "]";

    if (!file.empty() && line > 0) {
        formatted << " [" << extractFilename(file) << ":" << line << "]";
    }

    if (!module.empty()) {
        formatted << " [" << module << "]";
    }

    formatted << " " << message;

    const std::string entry = formatted.str();

    if (consoleEnabled_) {
        std::cout << entry << '\n';
        std::cout.flush();
    }

    if (fileEnabled_ && fileStream_ && fileStream_->is_open()) {
        *fileStream_ << entry << '\n';
        fileStream_->flush();
    }
}

std::string Logger::getTimestamp() const {
    const auto now = std::chrono::system_clock::now();
    const auto timeNow = std::chrono::system_clock::to_time_t(now);

    std::tm localTime {};
#if defined(_WIN32)
    localtime_s(&localTime, &timeNow);
#else
    localtime_r(&timeNow, &localTime);
#endif

    std::ostringstream timestamp;
    timestamp << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return timestamp.str();
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARN:
            return "WARN";
        case LogLevel::ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

std::string Logger::extractFilename(const std::string& path) const {
#ifdef _WIN32
    const size_t pos = path.find_last_of("\\/");
#else
    const size_t pos = path.find_last_of('/');
#endif
    if (pos == std::string::npos) {
        return path;
    }
    return path.substr(pos + 1);
}

void Logger::resetOutputsLocked() {
    fileEnabled_ = false;
    consoleEnabled_ = false;
    if (fileStream_ && fileStream_->is_open()) {
        fileStream_->close();
    }
    fileStream_.reset();
}

bool Logger::openLogFileLocked(const std::string& filename) {
    if (fileStream_ && fileStream_->is_open()) {
        fileStream_->close();
    }

    fileStream_ = std::make_unique<std::ofstream>(filename, std::ios::out | std::ios::app);
    if (!fileStream_->is_open()) {
        fileStream_.reset();
        return false;
    }
    return true;
}

std::string Logger::makeDefaultLogFilename() const {
    const auto now = std::chrono::system_clock::now();
    const auto timeNow = std::chrono::system_clock::to_time_t(now);

    std::tm localTime {};
#if defined(_WIN32)
    localtime_s(&localTime, &timeNow);
#else
    localtime_r(&timeNow, &localTime);
#endif

    std::ostringstream filename;
    filename << "log_"
             << (localTime.tm_year + 1900) << "-"
             << std::setfill('0') << std::setw(2) << (localTime.tm_mon + 1) << "-"
             << std::setfill('0') << std::setw(2) << localTime.tm_mday << "_"
             << std::setfill('0') << std::setw(2) << localTime.tm_hour << "-"
             << std::setfill('0') << std::setw(2) << localTime.tm_min << "-"
             << std::setfill('0') << std::setw(2) << localTime.tm_sec
             << ".txt";
    return filename.str();
}

}  // namespace myproject
