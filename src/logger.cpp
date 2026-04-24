#include "myproject/logger.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <process.h>
#else
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace myproject {
namespace {

std::mutex g_loggerMutex;
std::atomic<std::uint64_t> g_nextThreadNumber {1};

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

std::shared_ptr<std::once_flag> makeFreshOnceFlag() {
    return std::make_shared<std::once_flag>();
}

std::shared_ptr<std::once_flag> makeSatisfiedOnceFlag() {
    std::shared_ptr<std::once_flag> flag = makeFreshOnceFlag();
    std::call_once(*flag, [] {});
    return flag;
}

std::uint64_t currentThreadNumber() {
    thread_local const std::uint64_t threadNumber =
        g_nextThreadNumber.fetch_add(1, std::memory_order_relaxed);
    return threadNumber;
}

int currentProcessId() {
#if defined(_WIN32)
    return _getpid();
#else
    return static_cast<int>(getpid());
#endif
}

struct DaemonEndpoint {
    enum class Kind {
        kNone,
        kUnix,
        kTcp
    };

    Kind kind {Kind::kNone};
    std::string host;
    std::string port;
    std::string path;
};

std::vector<std::string> splitTokens(const std::string& value, char delimiter) {
    std::vector<std::string> tokens;
    std::size_t start = 0;
    while (start <= value.size()) {
        const std::size_t pos = value.find(delimiter, start);
        const std::size_t end = (pos == std::string::npos) ? value.size() : pos;
        if (end > start) {
            tokens.push_back(value.substr(start, end - start));
        }
        if (pos == std::string::npos) {
            break;
        }
        start = pos + 1;
    }
    return tokens;
}

DaemonEndpoint parseDaemonEndpoint(const std::string& addr) {
    if (addr.empty()) {
        return {};
    }

    if (addr.rfind("unix:", 0) == 0) {
        const std::string path = addr.substr(std::strlen("unix:"));
        if (path.empty()) {
            return {};
        }
        DaemonEndpoint endpoint;
        endpoint.kind = DaemonEndpoint::Kind::kUnix;
        endpoint.path = path;
        return endpoint;
    }

    if (!addr.empty() && addr[0] == '/') {
        DaemonEndpoint endpoint;
        endpoint.kind = DaemonEndpoint::Kind::kUnix;
        endpoint.path = addr;
        return endpoint;
    }

    const std::size_t colon = addr.rfind(':');
    if (colon == std::string::npos || colon == 0 || colon + 1 >= addr.size()) {
        return {};
    }

    DaemonEndpoint endpoint;
    endpoint.kind = DaemonEndpoint::Kind::kTcp;
    endpoint.host = addr.substr(0, colon);
    endpoint.port = addr.substr(colon + 1);
    return endpoint;
}

#if !defined(_WIN32)
bool setNonBlocking(int fd, bool enabled) {
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    const int updated = enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return fcntl(fd, F_SETFL, updated) == 0;
}

bool waitForConnect(int fd, std::chrono::milliseconds timeout) {
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(fd, &writeSet);

    struct timeval tv {};
    tv.tv_sec = static_cast<long>(timeout.count() / 1000);
    tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

    const int result = select(fd + 1, nullptr, &writeSet, nullptr, &tv);
    if (result <= 0) {
        return false;
    }

    int error = 0;
    socklen_t errorLen = sizeof(error);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &errorLen) != 0) {
        return false;
    }
    return error == 0;
}

void setSocketTimeouts(int fd, std::chrono::milliseconds timeout) {
    struct timeval tv {};
    tv.tv_sec = static_cast<long>(timeout.count() / 1000);
    tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

int connectTcp(const std::string& host, const std::string& port) {
    struct addrinfo hints {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* results = nullptr;
    const int gai = getaddrinfo(host.c_str(), port.c_str(), &hints, &results);
    if (gai != 0) {
        return -1;
    }

    int connectedFd = -1;
    for (struct addrinfo* rp = results; rp != nullptr; rp = rp->ai_next) {
        const int fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0) {
            continue;
        }

        if (!setNonBlocking(fd, true)) {
            close(fd);
            continue;
        }

        const int rc = connect(fd, rp->ai_addr, static_cast<socklen_t>(rp->ai_addrlen));
        if (rc == 0) {
            setNonBlocking(fd, false);
            setSocketTimeouts(fd, std::chrono::milliseconds(50));
            connectedFd = fd;
            break;
        }

        if (errno == EINPROGRESS && waitForConnect(fd, std::chrono::milliseconds(100))) {
            setNonBlocking(fd, false);
            setSocketTimeouts(fd, std::chrono::milliseconds(50));
            connectedFd = fd;
            break;
        }

        close(fd);
    }

    freeaddrinfo(results);
    return connectedFd;
}

int connectUnix(const std::string& path) {
    if (path.size() >= sizeof(sockaddr_un::sun_path)) {
        return -1;
    }

    const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    if (!setNonBlocking(fd, true)) {
        close(fd);
        return -1;
    }

    sockaddr_un addr {};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    const int rc = connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (rc == 0) {
        setNonBlocking(fd, false);
        setSocketTimeouts(fd, std::chrono::milliseconds(50));
        return fd;
    }

    if (errno == EINPROGRESS && waitForConnect(fd, std::chrono::milliseconds(100))) {
        setNonBlocking(fd, false);
        setSocketTimeouts(fd, std::chrono::milliseconds(50));
        return fd;
    }

    close(fd);
    return -1;
}
#endif

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
      fileEnabled_(false),
      daemonEnabled_(false),
      daemonAddr_(),
      daemonSocketFd_(std::nullopt),
      autoInitFlag_(makeFreshOnceFlag()) {
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
    std::lock_guard<std::mutex> lock(g_loggerMutex);

    resetOutputsLocked();
    shutdown_ = false;
    initialized_ = true;
    autoInitFlag_ = makeSatisfiedOnceFlag();

    minLevel_ = defaultLevel.value_or(LogLevel::WARN);

    const char* levelEnv = std::getenv("LOG_LEVEL");
    const std::optional<LogLevel> envLevel = parseLogLevelToken(levelEnv);
    if (envLevel.has_value()) {
        minLevel_ = *envLevel;
    } else if (levelEnv != nullptr && levelEnv[0] != '\0') {
        minLevel_ = LogLevel::WARN;
    }

    const std::string outputMode = normalizeToken(std::getenv("LOG_OUTPUT"));
    bool anyKnownToken = false;
    for (const std::string& token : splitTokens(outputMode, '+')) {
        if (token == "console") {
            consoleEnabled_ = true;
            anyKnownToken = true;
        } else if (token == "file") {
            fileEnabled_ = true;
            anyKnownToken = true;
        } else if (token == "both") {
            consoleEnabled_ = true;
            fileEnabled_ = true;
            anyKnownToken = true;
        } else if (token == "daemon") {
            daemonEnabled_ = true;
            anyKnownToken = true;
        }
    }

    if (!anyKnownToken) {
        fileEnabled_ = true;
    }

    if (fileEnabled_) {
        const char* fileEnv = std::getenv("LOG_FILE");
        const std::string path = (fileEnv != nullptr && fileEnv[0] != '\0')
            ? std::string(fileEnv)
            : makeDefaultLogFilename();

        if (!openLogFileLocked(path)) {
            fileEnabled_ = false;
            consoleEnabled_ = true;
        }
    }

    if (daemonEnabled_) {
        const char* addrEnv = std::getenv("LOG_DAEMON_ADDR");
        daemonAddr_ = (addrEnv != nullptr && addrEnv[0] != '\0') ? std::string(addrEnv) : "";
        if (!connectDaemonLocked()) {
            if (!consoleEnabled_ && !fileEnabled_) {
                consoleEnabled_ = true;
            }
            daemonEnabled_ = false;
        }
    }
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(g_loggerMutex);
    resetOutputsLocked();
    initialized_ = false;
    shutdown_ = true;
    autoInitFlag_ = makeSatisfiedOnceFlag();
}

bool Logger::enableFileLogging(const std::string& filename) {
    std::lock_guard<std::mutex> lock(g_loggerMutex);

    if (shutdown_) {
        shutdown_ = false;
        initialized_ = true;
    } else if (!initialized_) {
        initialized_ = true;
    }

    fileEnabled_ = openLogFileLocked(filename.empty() ? makeDefaultLogFilename() : filename);
    autoInitFlag_ = makeSatisfiedOnceFlag();
    return fileEnabled_;
}

void Logger::disableFileLogging() {
    std::lock_guard<std::mutex> lock(g_loggerMutex);
    fileEnabled_ = false;
    if (fileStream_ && fileStream_->is_open()) {
        fileStream_->close();
    }
    fileStream_.reset();
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(g_loggerMutex);
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
    std::shared_ptr<std::once_flag> autoInitFlag;
    {
        std::lock_guard<std::mutex> lock(g_loggerMutex);
        if (shutdown_) {
            return;
        }
        autoInitFlag = autoInitFlag_;
    }

    std::call_once(*autoInitFlag, [this]() {
        std::lock_guard<std::mutex> lock(g_loggerMutex);
        if (shutdown_ || initialized_) {
            return;
        }
        resetOutputsLocked();
        shutdown_ = false;
        initialized_ = true;

        minLevel_ = LogLevel::WARN;

        const char* levelEnv = std::getenv("LOG_LEVEL");
        const std::optional<LogLevel> envLevel = parseLogLevelToken(levelEnv);
        if (envLevel.has_value()) {
            minLevel_ = *envLevel;
        } else if (levelEnv != nullptr && levelEnv[0] != '\0') {
            minLevel_ = LogLevel::WARN;
        }

        const std::string outputMode = normalizeToken(std::getenv("LOG_OUTPUT"));
        bool anyKnownToken = false;
        for (const std::string& token : splitTokens(outputMode, '+')) {
            if (token == "console") {
                consoleEnabled_ = true;
                anyKnownToken = true;
            } else if (token == "file") {
                fileEnabled_ = true;
                anyKnownToken = true;
            } else if (token == "both") {
                consoleEnabled_ = true;
                fileEnabled_ = true;
                anyKnownToken = true;
            } else if (token == "daemon") {
                daemonEnabled_ = true;
                anyKnownToken = true;
            }
        }

        if (!anyKnownToken) {
            fileEnabled_ = true;
        }

        if (fileEnabled_) {
            const char* fileEnv = std::getenv("LOG_FILE");
            const std::string path = (fileEnv != nullptr && fileEnv[0] != '\0')
                ? std::string(fileEnv)
                : makeDefaultLogFilename();

            if (!openLogFileLocked(path)) {
                fileEnabled_ = false;
                consoleEnabled_ = true;
            }
        }

        if (daemonEnabled_) {
            const char* addrEnv = std::getenv("LOG_DAEMON_ADDR");
            daemonAddr_ = (addrEnv != nullptr && addrEnv[0] != '\0') ? std::string(addrEnv) : "";
            if (!connectDaemonLocked()) {
                if (!consoleEnabled_ && !fileEnabled_) {
                    consoleEnabled_ = true;
                }
                daemonEnabled_ = false;
            }
        }
    });

    std::lock_guard<std::mutex> lock(g_loggerMutex);
    if (shutdown_ || !initialized_ || level < minLevel_) {
        return;
    }

    logInternal(level, message, file, line, module);
}

void Logger::logInternal(LogLevel level, const std::string& message, const std::string& file, int line, const std::string& module) {
    const std::string entry = formatEntry(level, message, file, line, module);
    writeEntryLocked(entry);
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
    daemonEnabled_ = false;
    daemonAddr_.clear();
    disableDaemonLocked();
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

std::string Logger::formatEntry(
    LogLevel level,
    const std::string& message,
    const std::string& file,
    int line,
    const std::string& module) const {
    std::ostringstream formatted;
    formatted << "[" << getTimestamp() << "]"
              << " [" << levelToString(level) << "]"
              << " [pid=" << currentProcessId() << "]"
              << " [thread=" << currentThreadNumber() << "]";

    if (!file.empty() && line > 0) {
        formatted << " [" << extractFilename(file) << ":" << line << "]";
    }

    if (!module.empty()) {
        formatted << " [" << module << "]";
    }

    formatted << " " << message;
    return formatted.str();
}

void Logger::writeEntryLocked(const std::string& entry) {
    if (consoleEnabled_) {
        std::cout.write(entry.data(), static_cast<std::streamsize>(entry.size()));
        std::cout.put('\n');
        std::cout.flush();
    }

    if (fileEnabled_ && fileStream_ && fileStream_->is_open()) {
        fileStream_->write(entry.data(), static_cast<std::streamsize>(entry.size()));
        fileStream_->put('\n');
        fileStream_->flush();
    }

    if (daemonEnabled_) {
        if (!daemonSocketFd_.has_value() && !connectDaemonLocked()) {
            if (!consoleEnabled_ && !fileEnabled_) {
                consoleEnabled_ = true;
                std::cout.write(entry.data(), static_cast<std::streamsize>(entry.size()));
                std::cout.put('\n');
                std::cout.flush();
            }
            daemonEnabled_ = false;
            return;
        }

#if !defined(_WIN32)
        if (daemonSocketFd_.has_value()) {
            const std::string frame = entry + "\n";
            const int fd = *daemonSocketFd_;
            std::size_t offset = 0;
            while (offset < frame.size()) {
                const ssize_t sent = send(
                    fd,
                    frame.data() + offset,
                    frame.size() - offset,
                    MSG_NOSIGNAL);
                if (sent < 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    disableDaemonLocked();
                    if (!consoleEnabled_ && !fileEnabled_) {
                        consoleEnabled_ = true;
                        std::cout.write(entry.data(), static_cast<std::streamsize>(entry.size()));
                        std::cout.put('\n');
                        std::cout.flush();
                    }
                    daemonEnabled_ = false;
                    break;
                }
                if (sent == 0) {
                    break;
                }
                offset += static_cast<std::size_t>(sent);
            }
        }
#endif
    }
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

bool Logger::connectDaemonLocked() {
#if defined(_WIN32)
    (void)daemonAddr_;
    daemonSocketFd_.reset();
    return false;
#else
    if (daemonSocketFd_.has_value()) {
        return true;
    }

    const DaemonEndpoint endpoint = parseDaemonEndpoint(daemonAddr_);
    int fd = -1;
    if (endpoint.kind == DaemonEndpoint::Kind::kUnix) {
        fd = connectUnix(endpoint.path);
    } else if (endpoint.kind == DaemonEndpoint::Kind::kTcp) {
        fd = connectTcp(endpoint.host, endpoint.port);
    } else {
        return false;
    }

    if (fd < 0) {
        return false;
    }

    daemonSocketFd_ = fd;
    return true;
#endif
}

void Logger::disableDaemonLocked() {
#if defined(_WIN32)
    daemonSocketFd_.reset();
#else
    if (daemonSocketFd_.has_value()) {
        close(*daemonSocketFd_);
        daemonSocketFd_.reset();
    }
#endif
}

}  // namespace myproject
