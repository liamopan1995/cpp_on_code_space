#include <iostream>
#include "myproject/logger.h"

int main() {
    std::cout << "=== Logger 基础测试 ===" << std::endl;
    
    // 获取logger实例
    auto& logger = myproject::Logger::getInstance();
    std::cout << "1. Logger实例获取成功" << std::endl;
    
    // 初始化
    logger.init(myproject::LogLevel::DEBUG);
    std::cout << "2. Logger初始化完成" << std::endl;
    
    // 测试不同级别的日志
    logger.debug("调试信息");
    logger.info("普通信息");
    logger.warn("警告信息");
    logger.error("错误信息");
    
    std::cout << "3. 所有日志级别测试完成" << std::endl;
    
    // 测试日志级别过滤
    logger.setLogLevel(myproject::LogLevel::WARN);
    logger.info("这条信息不应该显示（级别是WARN）");
    logger.warn("这条警告应该显示");
    
    std::cout << "4. 日志级别过滤测试完成" << std::endl;
    
    // 测试文件日志
    logger.enableFileLogging("test.log");
    logger.info("这条信息会同时输出到控制台和文件");
    logger.disableFileLogging();
    
    std::cout << "5. 文件日志测试完成" << std::endl;
    
    std::cout << "=== Logger 测试结束 ===" << std::endl;
    return 0;
}