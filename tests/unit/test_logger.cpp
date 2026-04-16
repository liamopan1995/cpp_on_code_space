#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include "myproject/logger.h"

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 保存原始环境变量
        const char* logFile = std::getenv("LOG_FILE");
        const char* logLevel = std::getenv("LOG_LEVEL");
        const char* logOutput = std::getenv("LOG_OUTPUT");
        
        if (logFile) originalLogFile_ = logFile;
        if (logLevel) originalLogLevel_ = logLevel;
        if (logOutput) originalLogOutput_ = logOutput;
        
        // 清理环境变量
        unsetenv("LOG_FILE");
        unsetenv("LOG_LEVEL");
        unsetenv("LOG_OUTPUT");
        
        // 清理可能存在的测试文件
        std::remove("test_logger_output.log");
        std::remove("test_console_only.log");
    }
    
    void TearDown() override {
        // 恢复环境变量
        if (!originalLogFile_.empty()) {
            setenv("LOG_FILE", originalLogFile_.c_str(), 1);
        } else {
            unsetenv("LOG_FILE");
        }
        
        if (!originalLogLevel_.empty()) {
            setenv("LOG_LEVEL", originalLogLevel_.c_str(), 1);
        } else {
            unsetenv("LOG_LEVEL");
        }
        
        if (!originalLogOutput_.empty()) {
            setenv("LOG_OUTPUT", originalLogOutput_.c_str(), 1);
        } else {
            unsetenv("LOG_OUTPUT");
        }
        
        // 清理测试文件
        std::remove("test_logger_output.log");
        std::remove("test_console_only.log");
    }
    
    std::string readFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return "";
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
private:
    std::string originalLogFile_;
    std::string originalLogLevel_;
    std::string originalLogOutput_;
};

// 测试 3.1: LOG_LEVEL 默认为 WARNING 并过滤低级别消息
TEST_F(LoggerTest, LogLevelDefaultsToWarning) {
    auto& logger = myproject::Logger::getInstance();
    logger.init();
    
    // 重定向 cout 到 stringstream 以捕获输出
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    
    // 测试不同级别的日志
    logger.debug("Debug message should not appear");
    logger.info("Info message should not appear");
    logger.warn("Warning message should appear");
    logger.error("Error message should appear");
    
    std::cout.rdbuf(old); // 恢复 cout
    
    std::string output = buffer.str();
    
    // 验证 DEBUG 和 INFO 消息被过滤
    EXPECT_EQ(output.find("Debug message should not appear"), std::string::npos);
    EXPECT_EQ(output.find("Info message should not appear"), std::string::npos);
    
    // 验证 WARN 和 ERROR 消息出现
    EXPECT_NE(output.find("Warning message should appear"), std::string::npos);
    EXPECT_NE(output.find("Error message should appear"), std::string::npos);
}

// 测试 3.2: LOG_OUTPUT 控制台 vs 文件输出
TEST_F(LoggerTest, LogOutputControl) {
    // 测试 1: 默认情况下应该输出到控制台
    {
        auto& logger = myproject::Logger::getInstance();
        logger.init();
        
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        
        logger.info("Console output test");
        
        std::cout.rdbuf(old);
        
        EXPECT_EQ(buffer.str().find("Console output test"), std::string::npos);// info 级别的日志不应该被输出，因为默认级别是 WARN
    }
    
    // 测试 2: 启用文件日志后应该同时输出到文件
    {
        auto& logger = myproject::Logger::getInstance();
        logger.init();
        logger.enableFileLogging("test_logger_output.log");
        
        logger.warn("File logging test");
        logger.disableFileLogging();
        
        // 检查文件内容
        std::string fileContent = readFile("test_logger_output.log");
        EXPECT_NE(fileContent.find("File logging test"), std::string::npos);
    }
}
// 测试环境变量 LOG_LEVEL
TEST_F(LoggerTest, EnvironmentVariableLogLevel) {
    // 设置环境变量
    setenv("LOG_LEVEL", "ERROR", 1);
    
    auto& logger = myproject::Logger::getInstance();
    logger.init();
    
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    
    logger.debug("Debug - should not appear");
    logger.info("Info - should not appear");
    logger.warn("Warning - should not appear");
    logger.error("Error - should appear");
    
    std::cout.rdbuf(old);
    
    std::string output = buffer.str();
    
    // 验证只有 ERROR 级别消息出现
    EXPECT_EQ(output.find("Debug - should not appear"), std::string::npos);
    EXPECT_EQ(output.find("Info - should not appear"), std::string::npos);
    EXPECT_EQ(output.find("Warning - should not appear"), std::string::npos);
    EXPECT_NE(output.find("Error - should appear"), std::string::npos);
}

// 测试环境变量 LOG_FILE
TEST_F(LoggerTest, EnvironmentVariableLogFile) {
    // 设置环境变量
    setenv("LOG_FILE", "test_env_file.log", 1);
    
    auto& logger = myproject::Logger::getInstance();
    logger.init();
    
    logger.warn("Message via environment variable");
    
    // 检查文件是否被创建并包含消息
    std::string fileContent = readFile("test_env_file.log");
    EXPECT_NE(fileContent.find("Message via environment variable"), std::string::npos);
    
    // 清理
    std::remove("test_env_file.log");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}