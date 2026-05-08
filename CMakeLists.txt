cmake_minimum_required(VERSION 3.10)
project(CPP_ON_CODE_SPACE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files for main executable (now includes logger.cpp)
add_executable(main src/main.cpp src/calculator.cpp src/logger.cpp)
add_executable(logger_demo src/logger_demo.cpp src/logger.cpp)
add_executable(logdaemon src/logdaemon.cpp)
add_executable(logger_client_demo_tcp_socket src/logger_client_demo_tcp_socket.cpp)
add_executable(logger_client_demo_unix_domain_socket src/logger_client_demo_unix_domain_socket.cpp)


# Include directories
target_include_directories(main PRIVATE include)
target_include_directories(logger_demo PRIVATE include)
target_include_directories(logger_client_demo_tcp_socket PRIVATE include)
target_include_directories(logger_client_demo_unix_domain_socket PRIVATE include)


# Add tests
enable_testing()
add_subdirectory(tests)
