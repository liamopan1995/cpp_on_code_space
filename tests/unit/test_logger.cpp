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
        std::remove("test_env_file.log");
        std::remove("test_output_file.log");
        std::remove("demo_log.txt");
        
        // 重置 logger 状态
        // 由于 logger 是单例且没有重置方法，我们通过重新初始化来重置
        // 但 initialized_ 是私有的，所以我们只能依赖每个测试自己设置正确的状态
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
        std::remove("test_env_file.log");
        std::remove("test_output_file.log");
        std::remove("demo_log.txt");
    }
    
    std::string readFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return "";
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    // 辅助函数：从日志行提取文件名
    std::string extractFilename(const std::string& logLine) {
        // 查找第三个方括号对（文件:行信息）
        size_t pos = 0;
        for (int i = 0; i < 2; ++i) {
            pos = logLine.find("]", pos);
            if (pos == std::string::npos) return "";
            pos++;
        }
        
        // 现在pos指向第三个方括号的开始
        size_t start = logLine.find("[", pos);
        if (start == std::string::npos) return "";
        
        size_t end = logLine.find("]", start);
        if (end == std::string::npos) return "";
        
        return logLine.substr(start + 1, end - start - 1);
    }
    
private:
    std::string originalLogFile_;
    std::string originalLogLevel_;
    std::string originalLogOutput_;
};

// 测试 3.1: LOG_LEVEL 默认为 WARNING 并过滤低级别消息
TEST_F(LoggerTest, LogLevelDefaultsToWarning) {
    // 在初始化logger之前重定向输出
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    
    auto& logger = myproject::Logger::getInstance();

    logger.init();  // 默认级别是 WARN
    
    // 测试不同级别的日志（使用宏自动捕获文件行号）
    LOG_DEBUG("Debug message should not appear");
    LOG_INFO("Info message should not appear");
    LOG_WARN("Warning message should appear");
    LOG_ERROR("Error message should appear");
    
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
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        
        auto& logger = myproject::Logger::getInstance();

        // 重要：设置日志级别为 INFO，这样 INFO 消息不会被过滤
        logger.init(myproject::LogLevel::INFO);
        
        LOG_INFO("Console output test");
        
        std::cout.rdbuf(old);
        

        std::string captured = buffer.str();
        bool found = captured.find("Console output test") != std::string::npos;
        EXPECT_TRUE(found) << "没有找到 'Console output test'，捕获的输出是:\n" << captured;
    }
    
    // 测试 2: 启用文件日志后应该同时输出到文件
    {
        auto& logger = myproject::Logger::getInstance();

        // 重要：设置日志级别为 INFO
        logger.init(myproject::LogLevel::INFO);
        logger.enableFileLogging("test_logger_output.log");
        
        LOG_INFO("File logging test");
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
    
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    
    auto& logger = myproject::Logger::getInstance();

    logger.init();  // 会读取环境变量，设置级别为 ERROR
    
    LOG_DEBUG("Debug - should not appear");
    LOG_INFO("Info - should not appear");
    LOG_WARN("Warning - should not appear");
    LOG_ERROR("Error - should appear");
    
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

    // 重要：设置日志级别为 INFO
    logger.init(myproject::LogLevel::INFO);
    
    LOG_INFO("Message via environment variable");
    
    // 检查文件是否被创建并包含消息
    std::string fileContent = readFile("test_env_file.log");
    EXPECT_NE(fileContent.find("Message via environment variable"), std::string::npos);
    
    // 清理
    std::remove("test_env_file.log");
}

// 测试环境变量 LOG_OUTPUT
TEST_F(LoggerTest, EnvironmentVariableLogOutput) {
    // 测试 console 模式
    {
        setenv("LOG_OUTPUT", "console", 1);
        
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        
        auto& logger = myproject::Logger::getInstance();

        // 重要：设置日志级别为 INFO
        logger.init(myproject::LogLevel::INFO);
        
        LOG_INFO("Console only test");
        
        std::cout.rdbuf(old);
        

        std::string captured = buffer.str();
        bool found = captured.find("Console only test") != std::string::npos;
        EXPECT_TRUE(found) << "没有找到 'Console only test'，捕获的输出是:\n" << captured;
        
        unsetenv("LOG_OUTPUT");
    }
    
    // 测试 file 模式
    {
        setenv("LOG_OUTPUT", "file", 1);
        setenv("LOG_FILE", "test_output_file.log", 1);
        
        auto& logger = myproject::Logger::getInstance();

        // 重要：设置日志级别为 INFO
        logger.init(myproject::LogLevel::INFO);
        
        LOG_INFO("File only test");
        
        // 检查文件内容
        std::string fileContent = readFile("test_output_file.log");
        EXPECT_NE(fileContent.find("File only test"), std::string::npos);
        
        // 清理
        std::remove("test_output_file.log");
        unsetenv("LOG_OUTPUT");
        unsetenv("LOG_FILE");
    }
}
