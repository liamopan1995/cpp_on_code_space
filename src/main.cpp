#include <iostream>
#include "myproject/calculator.h"
#include "myproject/logger.h"

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

    LOG_INFO("Starting Calculator Demo");
    std::cout << "Calculator Demo:" << std::endl;
    
    LOG_DEBUG("Performing addition: " + std::to_string(a) + " + " + std::to_string(b));
    std::cout << a << " + " << b << " = " << myproject::Calculator::add(a, b) << std::endl;
    
    LOG_DEBUG("Performing subtraction: " + std::to_string(a) + " - " + std::to_string(b));
    std::cout << a << " - " << b << " = " << myproject::Calculator::subtract(a, b) << std::endl;
    
    LOG_DEBUG("Performing multiplication: " + std::to_string(a) + " * " + std::to_string(b));
    std::cout << a << " * " << b << " = " << myproject::Calculator::multiply(a, b) << std::endl;
    
    LOG_DEBUG("Performing division: " + std::to_string(a) + " / " + std::to_string(b));
    std::cout << a << " / " << b << " = " << myproject::Calculator::divide(a, b) << std::endl;

    // Test division by zero
    LOG_WARN("Testing division by zero");
    try {
        myproject::Calculator::divide(a, 0.0);
    } catch (const std::runtime_error& e) {
        LOG_ERROR("Division by zero error: " + std::string(e.what()));
        std::cout << "Error: " << e.what() << std::endl;
    }

    LOG_INFO("Calculator Demo Completed");
    std::cerr << "DEBUG: 程序结束" << std::endl;
    return 0;
}