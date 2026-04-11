cmake_minimum_required(VERSION 3.10)
project(CPP_ON_CODE_SPACE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files for main executable (now includes logger.cpp)
add_executable(main src/main.cpp src/calculator.cpp src/module1/foo.cpp src/logger.cpp)

# Include directories
target_include_directories(main PRIVATE include)
# Add tests
enable_testing()
add_subdirectory(tests)