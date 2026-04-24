#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <cerrno>
#endif

#include "myproject/logger.h"

namespace {

class CoutCapture {
public:
    CoutCapture() : old_(std::cout.rdbuf(buffer_.rdbuf())) {
    }

    ~CoutCapture() {
        std::cout.rdbuf(old_);
    }

    std::string str() const {
        return buffer_.str();
    }

private:
    std::ostringstream buffer_;
    std::streambuf* old_;
};

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void setOrClearEnv(const char* name, const std::string& value) {
    if (value.empty()) {
        unsetenv(name);
    } else {
        setenv(name, value.c_str(), 1);
    }
}

std::vector<std::string> splitNonEmptyLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    return lines;
}

#if !defined(_WIN32)
class DaemonServer {
public:
    DaemonServer() {
        listenFd_ = createListeningSocket();
        acceptThread_ = std::thread([this]() { acceptLoop(); });
    }

    ~DaemonServer() {
        running_.store(false, std::memory_order_relaxed);
        const int client = clientFd_.exchange(-1, std::memory_order_acq_rel);
        if (client >= 0) {
            shutdown(client, SHUT_RDWR);
            close(client);
        }
        if (listenFd_ >= 0) {
            close(listenFd_);
            listenFd_ = -1;
        }
        if (!unixPath_.empty()) {
            unlink(unixPath_.c_str());
            unixPath_.clear();
        }
        if (acceptThread_.joinable()) {
            acceptThread_.join();
        }
    }

    std::string address() const {
        return boundAddr_;
    }

    bool waitForSubstring(const std::string& needle, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, timeout, [&]() {
            for (const std::string& line : lines_) {
                if (line.find(needle) != std::string::npos) {
                    return true;
                }
            }
            return false;
        });
    }

private:
    int createListeningSocket() {
        const int udsFd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (udsFd >= 0) {
            unixPath_ = "/tmp/test_logdaemon_" + std::to_string(getpid()) + "_" +
                std::to_string(reinterpret_cast<std::uintptr_t>(this)) + ".sock";

            sockaddr_un addr {};
            addr.sun_family = AF_UNIX;
            std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", unixPath_.c_str());
            unlink(unixPath_.c_str());

            if (bind(udsFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0 &&
                listen(udsFd, 8) == 0) {
                boundAddr_ = "unix:" + unixPath_;
                return udsFd;
            }

            close(udsFd);
            unixPath_.clear();
        }

        struct addrinfo hints {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        struct addrinfo* results = nullptr;
        if (getaddrinfo("127.0.0.1", "0", &hints, &results) != 0) {
            return -1;
        }

        int listenFd = -1;
        for (struct addrinfo* rp = results; rp != nullptr; rp = rp->ai_next) {
            const int fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (fd < 0) {
                continue;
            }

            int reuse = 1;
            setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

            if (bind(fd, rp->ai_addr, static_cast<socklen_t>(rp->ai_addrlen)) != 0) {
                close(fd);
                continue;
            }

            if (listen(fd, 8) != 0) {
                close(fd);
                continue;
            }

            sockaddr_storage local {};
            socklen_t localLen = sizeof(local);
            if (getsockname(fd, reinterpret_cast<sockaddr*>(&local), &localLen) == 0) {
                char hostBuf[NI_MAXHOST] {};
                char serviceBuf[NI_MAXSERV] {};
                if (getnameinfo(reinterpret_cast<sockaddr*>(&local),
                                localLen,
                                hostBuf,
                                sizeof(hostBuf),
                                serviceBuf,
                                sizeof(serviceBuf),
                                NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
                    boundAddr_ = std::string(hostBuf) + ":" + std::string(serviceBuf);
                }
            }

            listenFd = fd;
            break;
        }

        freeaddrinfo(results);
        return listenFd;
    }

    void acceptLoop() {
        if (listenFd_ < 0) {
            return;
        }

        sockaddr_storage peer {};
        socklen_t peerLen = sizeof(peer);
        const int clientFd = accept(listenFd_, reinterpret_cast<sockaddr*>(&peer), &peerLen);
        if (clientFd < 0) {
            return;
        }
        clientFd_.store(clientFd, std::memory_order_release);

        struct timeval tv {};
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;
        setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        std::string buffer;
        buffer.resize(4096);
        std::string carry;

        while (running_.load(std::memory_order_relaxed)) {
            const ssize_t readBytes = read(clientFd, buffer.data(), buffer.size());
            if (readBytes < 0) {
                if (errno == EINTR) {
                    continue;
                }
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                break;
            }
            if (readBytes == 0) {
                break;
            }

            carry.append(buffer.data(), static_cast<std::size_t>(readBytes));
            std::size_t start = 0;
            while (true) {
                const std::size_t pos = carry.find('\n', start);
                if (pos == std::string::npos) {
                    break;
                }
                const std::string line = carry.substr(start, pos - start);
                start = pos + 1;
                if (!line.empty()) {
                    std::lock_guard<std::mutex> lock(mutex_);
                    lines_.push_back(line);
                    cv_.notify_all();
                }
            }
            if (start > 0) {
                carry.erase(0, start);
            }
        }

        close(clientFd);
        clientFd_.store(-1, std::memory_order_release);
    }

    std::atomic<bool> running_ {true};
    int listenFd_ {-1};
    std::atomic<int> clientFd_ {-1};
    std::string boundAddr_;
    std::string unixPath_;
    std::thread acceptThread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<std::string> lines_;
};
#endif

}  // namespace

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        saveEnv("LOG_FILE", originalLogFile_);
        saveEnv("LOG_LEVEL", originalLogLevel_);
        saveEnv("LOG_OUTPUT", originalLogOutput_);
        saveEnv("LOG_DAEMON_ADDR", originalLogDaemonAddr_);

        unsetenv("LOG_FILE");
        unsetenv("LOG_LEVEL");
        unsetenv("LOG_OUTPUT");
        unsetenv("LOG_DAEMON_ADDR");

        cleanupFiles();
        myproject::Logger::getInstance().shutdown();
    }

    void TearDown() override {
        myproject::Logger::getInstance().shutdown();
        setOrClearEnv("LOG_FILE", originalLogFile_);
        setOrClearEnv("LOG_LEVEL", originalLogLevel_);
        setOrClearEnv("LOG_OUTPUT", originalLogOutput_);
        setOrClearEnv("LOG_DAEMON_ADDR", originalLogDaemonAddr_);
        cleanupFiles();
    }

    void saveEnv(const char* name, std::string& target) {
        const char* value = std::getenv(name);
        target = value == nullptr ? "" : value;
    }

    void cleanupFiles() {
        std::remove("test_logger_output.log");
        std::remove("test_logger_default.log");
        std::remove("test_logger_reinit.log");
        std::remove("test_logger_shutdown.log");
        std::remove("test_logger_format.log");
        std::remove("test_logger_context.log");
        std::remove("test_logger_concurrent.log");
        std::remove("test_logger_runtime.log");
    }

    std::string originalLogFile_;
    std::string originalLogLevel_;
    std::string originalLogOutput_;
    std::string originalLogDaemonAddr_;
};

TEST_F(LoggerTest, LogLevelDefaultsToWarning) {
    setenv("LOG_OUTPUT", "console", 1);
    CoutCapture capture;

    auto& logger = myproject::Logger::getInstance();
    logger.init();

    LOG_INFO("info should be filtered");
    LOG_WARN("warn should be visible");
    LOG_ERROR("error should be visible");

    const std::string output = capture.str();
    EXPECT_EQ(output.find("info should be filtered"), std::string::npos);
    EXPECT_NE(output.find("warn should be visible"), std::string::npos);
    EXPECT_NE(output.find("error should be visible"), std::string::npos);
}

TEST_F(LoggerTest, LogLevelDebugEnvironmentAllowsDebugMessages) {
    setenv("LOG_OUTPUT", "console", 1);
    setenv("LOG_LEVEL", "debug", 1);
    CoutCapture capture;

    auto& logger = myproject::Logger::getInstance();
    logger.init();

    LOG_DEBUG("debug should be visible");

    EXPECT_NE(capture.str().find("debug should be visible"), std::string::npos);
}

TEST_F(LoggerTest, ConsoleOnlyOutputDoesNotWriteToFile) {
    setenv("LOG_OUTPUT", "console", 1);
    setenv("LOG_FILE", "test_logger_output.log", 1);
    CoutCapture capture;

    auto& logger = myproject::Logger::getInstance();
    logger.init(myproject::LogLevel::INFO);

    LOG_INFO("console only message");

    EXPECT_NE(capture.str().find("console only message"), std::string::npos);
    EXPECT_TRUE(readFile("test_logger_output.log").empty());
}

TEST_F(LoggerTest, FileOnlyOutputDoesNotWriteToConsole) {
    setenv("LOG_OUTPUT", "file", 1);
    setenv("LOG_FILE", "test_logger_output.log", 1);
    CoutCapture capture;

    auto& logger = myproject::Logger::getInstance();
    logger.init(myproject::LogLevel::INFO);

    LOG_INFO("file only message");

    EXPECT_TRUE(capture.str().empty());
    EXPECT_NE(readFile("test_logger_output.log").find("file only message"), std::string::npos);
}

TEST_F(LoggerTest, InvalidOutputFallsBackToFileLogging) {
    setenv("LOG_OUTPUT", "invalid", 1);
    setenv("LOG_FILE", "test_logger_default.log", 1);
    CoutCapture capture;

    auto& logger = myproject::Logger::getInstance();
    logger.init(myproject::LogLevel::INFO);

    LOG_INFO("default file fallback");

    EXPECT_TRUE(capture.str().empty());
    EXPECT_NE(readFile("test_logger_default.log").find("default file fallback"), std::string::npos);
}

TEST_F(LoggerTest, InvalidLogLevelFallsBackToWarning) {
    setenv("LOG_OUTPUT", "console", 1);
    setenv("LOG_LEVEL", "invalid", 1);
    CoutCapture capture;

    auto& logger = myproject::Logger::getInstance();
    logger.init();

    LOG_INFO("info should still be filtered");
    LOG_WARN("warn should still pass");

    const std::string output = capture.str();
    EXPECT_EQ(output.find("info should still be filtered"), std::string::npos);
    EXPECT_NE(output.find("warn should still pass"), std::string::npos);
}

TEST_F(LoggerTest, LogEntryContainsTimestampFileLineAndModuleWhenProvided) {
    setenv("LOG_OUTPUT", "file", 1);
    setenv("LOG_FILE", "test_logger_format.log", 1);

    auto& logger = myproject::Logger::getInstance();
    logger.init(myproject::LogLevel::INFO);

    LOG_INFO_MODULE("formatted message", "FormatModule");

    const std::string contents = readFile("test_logger_format.log");
    const std::regex pattern(
        R"(\[[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}\] \[INFO\] \[pid=[0-9]+\] \[thread=[0-9]+\] \[test_logger\.cpp:[0-9]+\] \[FormatModule\] formatted message)");
    EXPECT_TRUE(std::regex_search(contents, pattern)) << contents;
}

TEST_F(LoggerTest, LogEntryContainsProcessAndThreadContextOnConsoleAndFile) {
    setenv("LOG_OUTPUT", "both", 1);
    setenv("LOG_FILE", "test_logger_context.log", 1);
    CoutCapture capture;

    auto& logger = myproject::Logger::getInstance();
    logger.init(myproject::LogLevel::INFO);

    LOG_INFO_MODULE("context message", "ContextModule");

    const std::regex pattern(
        R"(\[[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}\] \[INFO\] \[pid=[0-9]+\] \[thread=[0-9]+\] \[test_logger\.cpp:[0-9]+\] \[ContextModule\] context message)");
    const std::string consoleOutput = capture.str();
    const std::string fileOutput = readFile("test_logger_context.log");

    EXPECT_TRUE(std::regex_search(consoleOutput, pattern)) << consoleOutput;
    EXPECT_TRUE(std::regex_search(fileOutput, pattern)) << fileOutput;
}

TEST_F(LoggerTest, ShutdownPreventsFurtherWritesUntilReinitialized) {
    setenv("LOG_OUTPUT", "both", 1);
    setenv("LOG_FILE", "test_logger_shutdown.log", 1);
    CoutCapture capture;

    auto& logger = myproject::Logger::getInstance();
    logger.init(myproject::LogLevel::INFO);
    LOG_INFO("before shutdown");

    logger.shutdown();
    LOG_INFO("after shutdown");

    const std::string output = capture.str();
    const std::string contents = readFile("test_logger_shutdown.log");

    EXPECT_NE(output.find("before shutdown"), std::string::npos);
    EXPECT_EQ(output.find("after shutdown"), std::string::npos);
    EXPECT_NE(contents.find("before shutdown"), std::string::npos);
    EXPECT_EQ(contents.find("after shutdown"), std::string::npos);
}

TEST_F(LoggerTest, ReinitializationResetsLevelAndOutputDestination) {
    auto& logger = myproject::Logger::getInstance();

    setenv("LOG_OUTPUT", "console", 1);
    CoutCapture capture;
    logger.init(myproject::LogLevel::INFO);
    LOG_INFO("console pass");

    setenv("LOG_OUTPUT", "file", 1);
    setenv("LOG_LEVEL", "error", 1);
    setenv("LOG_FILE", "test_logger_reinit.log", 1);
    logger.init(myproject::LogLevel::DEBUG);
    LOG_INFO("file filtered");
    LOG_ERROR("file pass");

    const std::string output = capture.str();
    const std::string contents = readFile("test_logger_reinit.log");

    EXPECT_NE(output.find("console pass"), std::string::npos);
    EXPECT_EQ(output.find("file pass"), std::string::npos);
    EXPECT_EQ(contents.find("file filtered"), std::string::npos);
    EXPECT_NE(contents.find("file pass"), std::string::npos);
}

TEST_F(LoggerTest, RuntimeFileLoggingCanBeEnabledWithExplicitPath) {
    setenv("LOG_OUTPUT", "console", 1);
    CoutCapture capture;

    auto& logger = myproject::Logger::getInstance();
    logger.init(myproject::LogLevel::INFO);
    ASSERT_TRUE(logger.enableFileLogging("test_logger_runtime.log"));

    LOG_INFO("runtime file message");

    EXPECT_NE(capture.str().find("runtime file message"), std::string::npos);
    EXPECT_NE(readFile("test_logger_runtime.log").find("runtime file message"), std::string::npos);
}

TEST_F(LoggerTest, RuntimeFileLoggingFailureDoesNotBreakConsoleLogging) {
    setenv("LOG_OUTPUT", "console", 1);
    CoutCapture capture;

    auto& logger = myproject::Logger::getInstance();
    logger.init(myproject::LogLevel::INFO);

    EXPECT_FALSE(logger.enableFileLogging("missing-dir/test_logger_runtime.log"));
    LOG_INFO("console still works");

    EXPECT_NE(capture.str().find("console still works"), std::string::npos);
}

TEST_F(LoggerTest, ConcurrentLoggingProducesCompleteSerializedLinesOnConsoleAndFile) {
    setenv("LOG_OUTPUT", "both", 1);
    setenv("LOG_FILE", "test_logger_concurrent.log", 1);
    CoutCapture capture;

    auto& logger = myproject::Logger::getInstance();
    logger.init(myproject::LogLevel::INFO);

    constexpr int kThreadCount = 4;
    constexpr int kMessagesPerThread = 20;
    std::atomic<bool> startLogging {false};
    std::vector<std::thread> workers;
    workers.reserve(kThreadCount);

    for (int threadIndex = 0; threadIndex < kThreadCount; ++threadIndex) {
        workers.emplace_back([&logger, &startLogging, threadIndex]() {
            while (!startLogging.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            for (int messageIndex = 0; messageIndex < kMessagesPerThread; ++messageIndex) {
                logger.info(
                    "concurrent t" + std::to_string(threadIndex) + "-m" + std::to_string(messageIndex),
                    __FILE__,
                    __LINE__,
                    "Concurrent");
            }
        });
    }

    startLogging.store(true, std::memory_order_release);
    for (std::thread& worker : workers) {
        worker.join();
    }

    const std::regex linePattern(
        R"(^\[[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}\] \[INFO\] \[pid=([0-9]+)\] \[thread=([0-9]+)\] \[test_logger\.cpp:[0-9]+\] \[Concurrent\] (concurrent t[0-9]+-m[0-9]+)$)");

    std::set<std::string> expectedMessages;
    for (int threadIndex = 0; threadIndex < kThreadCount; ++threadIndex) {
        for (int messageIndex = 0; messageIndex < kMessagesPerThread; ++messageIndex) {
            expectedMessages.insert(
                "concurrent t" + std::to_string(threadIndex) + "-m" + std::to_string(messageIndex));
        }
    }

    auto validateOutput = [&](const std::string& output) {
        const std::vector<std::string> lines = splitNonEmptyLines(output);
        std::set<std::string> observedMessages;
        std::set<std::string> observedThreads;

        EXPECT_EQ(lines.size(), expectedMessages.size()) << output;

        for (const std::string& line : lines) {
            std::smatch match;
            const bool matched = std::regex_match(line, match, linePattern);
            EXPECT_TRUE(matched) << line;
            if (!matched) {
                continue;
            }
            observedThreads.insert(match[2].str());
            observedMessages.insert(match[3].str());
        }

        EXPECT_EQ(observedMessages, expectedMessages);
        EXPECT_EQ(observedThreads.size(), static_cast<std::size_t>(kThreadCount));
    };

    validateOutput(capture.str());
    validateOutput(readFile("test_logger_concurrent.log"));
}

#if !defined(_WIN32)
TEST_F(LoggerTest, DaemonOutputSendsEntriesToDaemonServer) {
    DaemonServer server;
    if (server.address().empty()) {
        GTEST_SKIP() << "No local IPC transport available for daemon test";
    }

    setenv("LOG_OUTPUT", "daemon", 1);
    setenv("LOG_DAEMON_ADDR", server.address().c_str(), 1);
    CoutCapture capture;

    auto& logger = myproject::Logger::getInstance();
    logger.init(myproject::LogLevel::INFO);

    LOG_INFO("daemon message");

    EXPECT_TRUE(capture.str().empty()) << capture.str();
    EXPECT_TRUE(server.waitForSubstring("daemon message", std::chrono::milliseconds(500)));
}

TEST_F(LoggerTest, DaemonUnreachableFallsBackToConsoleWithoutBlocking) {
    setenv("LOG_OUTPUT", "daemon", 1);
    setenv("LOG_DAEMON_ADDR", "127.0.0.1:1", 1);  // unlikely to be listening
    CoutCapture capture;

    auto& logger = myproject::Logger::getInstance();
    logger.init(myproject::LogLevel::INFO);

    LOG_INFO("fallback message");

    EXPECT_NE(capture.str().find("fallback message"), std::string::npos);
}
#endif
