#include "myproject/logger.h"
#include <thread>
#include <chrono>

void demoBasicLogging() {
    LOG_INFO("=== Basic Logging Demo ===");
    
    // Different log levels
    LOG_DEBUG("This is a debug message");
    LOG_INFO("This is an info message");
    LOG_WARN("This is a warning message");
    LOG_ERROR("This is an error message");
    
    LOG_INFO("=== End Basic Demo ===");
}

void demoLogLevelControl() {
    LOG_INFO("=== Log Level Control Demo ===");
    
    auto& logger = myproject::Logger::getInstance();
    
    // Show current level
    LOG_INFO("Current log level shows all messages above INFO");
    LOG_DEBUG("This debug message should appear");
    
    // Change to WARN level
    logger.setLogLevel(myproject::LogLevel::WARN);
    LOG_INFO("This info message should NOT appear now");
    LOG_WARN("This warning message should appear");
    LOG_ERROR("This error message should appear");
    
    // Change back to DEBUG level
    logger.setLogLevel(myproject::LogLevel::DEBUG);
    LOG_DEBUG("Debug messages are visible again");
    
    LOG_INFO("=== End Log Level Demo ===");
}

void demoFileLogging() {
    LOG_INFO("=== File Logging Demo ===");
    
    auto& logger = myproject::Logger::getInstance();
    
    // Enable file logging with auto-generated filename
    logger.enableFileLogging();
    
    LOG_INFO("This message will be written to both console and file");
    LOG_WARN("Warning messages also go to file");
    LOG_ERROR("Error messages are logged to file too");
    
    // Disable file logging
    logger.disableFileLogging();
    LOG_INFO("File logging disabled - messages only go to console now");
    
    LOG_INFO("=== End File Logging Demo ===");
}

void demoThreadSafety() {
    LOG_INFO("=== Thread Safety Demo ===");
    
    auto logFromThread = [](int threadId) {
        for (int i = 0; i < 5; ++i) {
            LOG_INFO("Thread " + std::to_string(threadId) + " - Message " + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };
    
    std::thread t1(logFromThread, 1);
    std::thread t2(logFromThread, 2);
    std::thread t3(logFromThread, 3);
    
    t1.join();
    t2.join();
    t3.join();
    
    LOG_INFO("=== End Thread Safety Demo ===");
}

int main() {
    // Initialize logger (optional - will auto-initialize if not called)
    auto& logger = myproject::Logger::getInstance();
    logger.init(myproject::LogLevel::DEBUG);
    
    LOG_INFO("Starting Logger Demo Application");
    
    // Run demos
    demoBasicLogging();
    demoLogLevelControl();
    demoFileLogging();
    demoThreadSafety();
    
    LOG_INFO("Logger Demo Completed Successfully");
    
    return 0;
}