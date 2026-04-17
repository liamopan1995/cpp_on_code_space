#include <iostream>
#include <thread>
#include <vector>
#include "myproject/calculator.h"
#include "myproject/logger.h"

void threadFunction(int id) {
    for (int i = 0; i < 3; ++i) {
        LOG_INFO_MODULE("Thread " + std::to_string(id) + " - Message " + std::to_string(i), "ThreadDemo");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    // 添加调试输出
    std::cerr << "DEBUG: 程序开始" << std::endl;
    
    // Initialize logger
    auto& logger = myproject::Logger::getInstance();
    std::cerr << "DEBUG: 获取logger实例" << std::endl;
    
    logger.init(myproject::LogLevel::INFO);
    std::cerr << "DEBUG: logger初始化完成" << std::endl;
    
    LOG_INFO("Starting Calculator Application");
    std::cerr << "DEBUG: 第一条日志已记录" << std::endl;
    
    std::cout << "Hello, World!" << std::endl;

    // Calculator demonstration
    double a = 10.0, b = 5.0;

    LOG_INFO_MODULE("Starting Calculator Demo", "Calculator");
    std::cout << "Calculator Demo:" << std::endl;
    
    LOG_DEBUG_MODULE("Performing addition: " + std::to_string(a) + " + " + std::to_string(b), "Calculator");
    std::cout << a << " + " << b << " = " << myproject::Calculator::add(a, b) << std::endl;
    
    LOG_DEBUG_MODULE("Performing subtraction: " + std::to_string(a) + " - " + std::to_string(b), "Calculator");
    std::cout << a << " - " << b << " = " << myproject::Calculator::subtract(a, b) << std::endl;
    
    LOG_DEBUG_MODULE("Performing multiplication: " + std::to_string(a) + " * " + std::to_string(b), "Calculator");
    std::cout << a << " * " << b << " = " << myproject::Calculator::multiply(a, b) << std::endl;
    
    LOG_DEBUG_MODULE("Performing division: " + std::to_string(a) + " / " + std::to_string(b), "Calculator");
    std::cout << a << " / " << b << " = " << myproject::Calculator::divide(a, b) << std::endl;

    // Test division by zero
    LOG_WARN_MODULE("Testing division by zero", "Calculator");
    try {
        myproject::Calculator::divide(a, 0.0);
    } catch (const std::runtime_error& e) {
        LOG_ERROR_MODULE("Division by zero error: " + std::string(e.what()), "Calculator");
        std::cout << "Error: " << e.what() << std::endl;
    }

    LOG_INFO_MODULE("Calculator Demo Completed", "Calculator");

    // Enhanced logger features demonstration
    std::cout << "\n=== Enhanced Logger Features Demo ===" << std::endl;
    
    // Demo 1: Basic logging with file and line
    LOG_INFO("This message includes file and line number automatically");
    
    // Demo 2: Logging with module name
    LOG_INFO_MODULE("This message includes module name", "DemoModule");
    
    // Demo 3: Different log levels with context
    LOG_DEBUG_MODULE("Debug message with context", "TestModule");
    LOG_INFO_MODULE("Info message with context", "TestModule");
    LOG_WARN_MODULE("Warning message with context", "TestModule");
    LOG_ERROR_MODULE("Error message with context", "TestModule");
    
    // Demo 4: File logging
    std::cout << "\n=== File Logging Demo ===" << std::endl;
    logger.enableFileLogging("demo_log.txt");
    LOG_INFO_MODULE("This message goes to both console and file", "FileDemo");
    LOG_INFO_MODULE("Another file log message", "FileDemo");
    logger.disableFileLogging();
    LOG_INFO_MODULE("File logging disabled - messages only go to console now", "FileDemo");
    
    // Demo 5: Environment variable demonstration
    std::cout << "\n=== Environment Variable Demo ===" << std::endl;
    LOG_INFO("Try setting these environment variables:");
    LOG_INFO("  LOG_LEVEL=DEBUG - to see all messages");
    LOG_INFO("  LOG_OUTPUT=console - for console only");
    LOG_INFO("  LOG_OUTPUT=file - for file only");
    LOG_INFO("  LOG_FILE=custom.log - to specify log file");
    
    // Demo 6: Thread safety demonstration
    std::cout << "\n=== Thread Safety Demo ===" << std::endl;
    std::vector<std::thread> threads;
    for (int i = 1; i <= 3; ++i) {
        threads.emplace_back(threadFunction, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    LOG_INFO_MODULE("=== End Thread Safety Demo ===", "ThreadDemo");
    LOG_INFO("Application completed successfully");
    
    return 0;
}