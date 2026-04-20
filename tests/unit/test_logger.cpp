#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>

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

}  // namespace

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        saveEnv("LOG_FILE", originalLogFile_);
        saveEnv("LOG_LEVEL", originalLogLevel_);
        saveEnv("LOG_OUTPUT", originalLogOutput_);

        unsetenv("LOG_FILE");
        unsetenv("LOG_LEVEL");
        unsetenv("LOG_OUTPUT");

        cleanupFiles();
        myproject::Logger::getInstance().shutdown();
    }

    void TearDown() override {
        myproject::Logger::getInstance().shutdown();
        setOrClearEnv("LOG_FILE", originalLogFile_);
        setOrClearEnv("LOG_LEVEL", originalLogLevel_);
        setOrClearEnv("LOG_OUTPUT", originalLogOutput_);
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
        std::remove("test_logger_runtime.log");
    }

    std::string originalLogFile_;
    std::string originalLogLevel_;
    std::string originalLogOutput_;
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
        R"(\[[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}\] \[INFO\] \[test_logger\.cpp:[0-9]+\] \[FormatModule\] formatted message)");
    EXPECT_TRUE(std::regex_search(contents, pattern)) << contents;
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
