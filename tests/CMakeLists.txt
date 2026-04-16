# CMakeLists.txt for tests
# Add your test configurations here

# 首先尝试查找 gtest
find_package(GTest QUIET)

# Calculator test
add_executable(test_calculator unit/test_calculator.cpp ../src/calculator.cpp)
target_include_directories(test_calculator PRIVATE ../include)
target_link_libraries(test_calculator)  # No special libs needed

add_test(NAME test_calculator COMMAND test_calculator)

# Logger test
add_executable(test_logger unit/test_logger.cpp ../src/logger.cpp)
target_include_directories(test_logger PRIVATE ../include)

# 添加静态链接选项以避免 libstdc++ 版本问题
if(GTest_FOUND)
    message(STATUS "Found GTest, linking with GTest::GTest and GTest::Main")
    target_link_libraries(test_logger GTest::GTest GTest::Main)
else()
    message(STATUS "GTest not found, trying to link with gtest and gtest_main directly")
    # 尝试直接链接
    target_link_libraries(test_logger gtest gtest_main pthread)
endif()

# 添加链接器选项：使用静态链接或指定库路径
target_link_options(test_logger PRIVATE
    "-Wl,-rpath,/usr/lib/x86_64-linux-gnu"  # 指定运行时库路径
    "-static-libstdc++"  # 静态链接 libstdc++
    "-static-libgcc"     # 静态链接 libgcc
)

add_test(NAME test_logger COMMAND test_logger)

